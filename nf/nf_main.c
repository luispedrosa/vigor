#include <inttypes.h>

// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>

#ifdef KLEE_VERIFICATION
#  include <klee/klee.h>
#  include "lib/stubs/rte_stubs.h"
#  include "lib/flowmanager.h"
#  include "lib/stubs/loop.h"
#  include "lib/stubs/my-time-stub-control.h"
#else//KLEE_VERIFICATION
#  include <rte_common.h>
#  include <rte_eal.h>
#  include <rte_ethdev.h>
#  include <rte_launch.h>
#  include <rte_lcore.h>
#  include <rte_mbuf.h>
#endif//KLEE_VERIFICATION

#include "lib/nf_config.h"
#include "lib/nf_forward.h"
#include "lib/nf_log.h"
#include "lib/nf_time.h"
#include "lib/nf_util.h"
#include <string.h>


// --- Static config ---
// TODO see remark in lcore_main
// Size of batches to receive; trade-off between latency and throughput
// Can be overriden at compile time
//#ifndef BATCH_SIZE
//static const uint16_t BATCH_SIZE = 32;
//#endif

// Queue sizes for receiving/transmitting packets (set to their values from l3fwd sample)
static const uint16_t RX_QUEUE_SIZE = 128;
static const uint16_t TX_QUEUE_SIZE = 512;

// Memory pool #buffers and per-core cache size (set to their values from l3fwd sample)
static const unsigned MEMPOOL_BUFFER_COUNT = 8192;
static const unsigned MEMPOOL_CACHE_SIZE = 256;


// --- Initialization ---

static void
nat_print_config(struct nat_config* config)
{
	NF_INFO("\n--- NAT Config ---\n");

// TODO see remark in lcore_main
//	NF_INFO("Batch size: %" PRIu16, BATCH_SIZE);

	NF_INFO("Devices mask: 0x%" PRIx32, config->devices_mask);
	NF_INFO("Main LAN device: %" PRIu8, config->lan_main_device);
	NF_INFO("WAN device: %" PRIu8, config->wan_device);

	char* ext_ip_str = nf_ipv4_to_str(config->external_addr);
	NF_INFO("External IP: %s", ext_ip_str);
	free(ext_ip_str);

	uint8_t nb_devices = rte_eth_dev_count();
	for (uint8_t dev = 0; dev < nb_devices; dev++) {
		char* dev_mac_str = nf_mac_to_str(&(config->device_macs[dev]));
		char* end_mac_str = nf_mac_to_str(&(config->endpoint_macs[dev]));

		NF_INFO("Device %" PRIu8 " own-mac: %s, end-mac: %s", dev, dev_mac_str, end_mac_str);

		free(dev_mac_str);
		free(end_mac_str);
	}

	NF_INFO("Starting port: %" PRIu16, config->start_port);
	NF_INFO("Expiration time: %" PRIu32, config->expiration_time);
	NF_INFO("Max flows: %" PRIu16, config->max_flows);

	NF_INFO("\n--- --- ------ ---\n");
}

static int
nf_init_device(uint8_t device, struct rte_mempool *mbuf_pool)
{
	int retval;

	// Configure the device
	// This is ugly code; DPDK samples use designated initializers,
	// but those are not available in C++, and this code needs to compile
	// both as C and C++.
	struct rte_eth_conf device_conf;
	memset(&device_conf, 0, sizeof(struct rte_eth_conf));
	device_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
	device_conf.rxmode.max_rx_pkt_len = ETHER_MAX_LEN;
	device_conf.rxmode.split_hdr_size = 0;
	device_conf.rxmode.header_split =   0;
	device_conf.rxmode.hw_ip_checksum = 1;
	device_conf.rxmode.hw_vlan_filter = 0;
	device_conf.rxmode.jumbo_frame =    0;
	device_conf.rxmode.hw_strip_crc =   0;
	device_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
	device_conf.rx_adv_conf.rss_conf.rss_key = NULL;
	device_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP;

	retval = rte_eth_dev_configure(
		device, // The device
		1, // # of RX queues
		1, // # of TX queues
		&device_conf // device config
	);
	if (retval != 0) {
		rte_exit(EXIT_FAILURE, "Cannot configure device %" PRIu8 ", err=%d", device, retval);
	}

	// Allocate and set up 1 RX queue per device
	retval = rte_eth_rx_queue_setup(
		device, // device ID
		0, // queue ID
		RX_QUEUE_SIZE, // size
		rte_eth_dev_socket_id(device), // socket
		NULL, // config (NULL = default)
		mbuf_pool // memory pool
	);
	if (retval < 0) {
		rte_exit(EXIT_FAILURE, "Cannot allocate RX queue for device %" PRIu8 ", err=%d", device, retval);
	}

	// Allocate and set up 1 TX queue per device
	retval = rte_eth_tx_queue_setup(
		device, // device ID
		0, // queue ID
		TX_QUEUE_SIZE, // size
		rte_eth_dev_socket_id(device), // socket
		NULL // config (NULL = default)
	);
	if (retval < 0) {
		rte_exit(EXIT_FAILURE, "Cannot allocate TX queue for device %" PRIu8 " err=%d", device, retval);
	}

	// Start the device
	retval = rte_eth_dev_start(device);
	if (retval < 0) {
		rte_exit(EXIT_FAILURE, "Cannot start device on device %" PRIu8 ", err=%d", device, retval);
	}

	// Enable RX in promiscuous mode for the Ethernet device
	rte_eth_promiscuous_enable(device);

	return 0;
}


// --- Per-core work ---

//static __attribute__((noreturn))
void
lcore_main(struct nat_config* config)
{
	uint8_t nb_devices = rte_eth_dev_count();

	for (uint8_t device = 0; device < nb_devices; device++) {
		if (rte_eth_dev_socket_id(device) > 0 && rte_eth_dev_socket_id(device) != (int) rte_socket_id()) {
			NF_INFO("Device %" PRIu8 " is on remote NUMA node to polling thread.", device);
		}
	}

	nf_core_init(config);

	NF_INFO("Core %u forwarding packets.", rte_lcore_id());

	// Run until the application is killed
#ifdef KLEE_VERIFICATION
	uint32_t starting_time = start_time();
	unsigned lcore_id = rte_lcore_id(); // TODO do we need that?

	int x = klee_int("loop_termination");
	loop_iteration_begin(get_dmap_pp(), get_dchain_pp(), lcore_id, starting_time, config->max_flows, config->start_port);
  while (klee_induce_invariants() & x) {
    uint8_t device = klee_range(0, nb_devices, "device");
    {
      loop_iteration_assumptions(get_dmap_pp(), get_dchain_pp(), lcore_id, starting_time, config->max_flows, config->start_port);
#else //KLEE_VERIFICATION
	while (1) {
		for (uint8_t device = 0; device < nb_devices; device++) {
			if ((config->devices_mask & (1 << device)) == 0) {
				continue;
			}

#endif //KLEE_VERIFICATION
      uint32_t now = current_time();

			struct rte_mbuf* buf[1];
			uint16_t actual_rx_len = rte_eth_rx_burst(device, 0, buf, 1);

			if (actual_rx_len != 0) {
				uint8_t dst_device = nf_core_process(config, device, buf[0], now);

				if (dst_device == device) {
					rte_pktmbuf_free(buf[0]);
				} else {
					uint16_t actual_tx_len = rte_eth_tx_burst(dst_device, 0, buf, 1);

					if (actual_tx_len == 0) {
						rte_pktmbuf_free(buf[0]);
					}
				}
			}
// TODO benchmark, consider batching
//			struct rte_mbuf* bufs[BATCH_SIZE];
//			uint16_t bufs_len = rte_eth_rx_burst(device, 0, bufs, BATCH_SIZE);

//			if (likely(bufs_len != 0)) {
//				nf_core_process(config, core_id, device, bufs, bufs_len);
//			}
#ifdef KLEE_VERIFICATION
      loop_iteration_end(get_dmap_pp(), get_dchain_pp(), lcore_id, now, config->max_flows, config->start_port);
#endif//KLEE_VERIFICATION
		}
	}
}


// --- Main ---

int
main(int argc, char* argv[])
{
	// Initialize the Environment Abstraction Layer (EAL)
	int ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Error with EAL initialization, ret=%d\n", ret);
	}
	argc -= ret;
	argv += ret;

	struct nat_config config;
	nat_config_init(&config, argc, argv);
	nat_print_config(&config);

	// Create a memory pool
	unsigned nb_devices = rte_eth_dev_count();
	struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
		"MEMPOOL", // name
		MEMPOOL_BUFFER_COUNT * nb_devices, // #elements
		MEMPOOL_CACHE_SIZE, // cache size
		0, // application private area size
		RTE_MBUF_DEFAULT_BUF_SIZE, // data buffer size
		rte_socket_id() // socket ID
	);
	if (mbuf_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}

	// Initialize all devices
	for (uint8_t device = 0; device < nb_devices; device++) {
		if ((config.devices_mask & (1 << device)) == 0) {
			NF_INFO("Skipping disabled device %" PRIu8 ".", device);
		} else if (nf_init_device(device, mbuf_pool) == 0) {
			NF_INFO("Initialized device %" PRIu8 ".", device);
		} else {
			rte_exit(EXIT_FAILURE, "Cannot init device %" PRIu8 ".", device);
		}
	}

	// Run!
	// ...in single-threaded mode, that is.
	lcore_main(&config);

	return 0;
}
