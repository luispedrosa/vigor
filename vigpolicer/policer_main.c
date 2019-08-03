#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_byteorder.h>

#include "lib/nf_forward.h"
#include "lib/nf_util.h"
#include "lib/nf_log.h"
#include "policer_config.h"
#include "state.h"

#include "lib/containers/double-chain.h"
#include "lib/containers/map.h"
#include "lib/containers/vector.h"
#include "lib/expirator.h"

struct policer_config config;

struct State* dynamic_ft;

int policer_expire_entries(vigor_time_t time) {
  vigor_time_t exp_time = VIGOR_TIME_SECONDS_MULTIPLIER * config.burst / config.rate;

  if (time < exp_time)
    return 0;

  uint64_t time_u = (uint64_t) time;
  // OK because time >= config.burst / config.rate >= 0
  vigor_time_t min_time = time_u - exp_time;

  return expire_items_single_map(dynamic_ft->dyn_heap, dynamic_ft->dyn_keys,
                                 dynamic_ft->dyn_map,
                                 min_time);
}

bool
dyn_val_condition(void* key, int index, void* state) {
  return 0 <= ((struct DynamicValue*) key)->bucket_time AND
    ((struct DynamicValue*) key)->bucket_time <= recent_time() AND
    ((struct DynamicValue*) key)->bucket_size <= config.burst;
}

bool policer_check_tb(uint32_t dst, uint16_t size, vigor_time_t time) {
  int index = -1;
  int present = map_get(dynamic_ft->dyn_map, &dst, &index);
  if (present) {
    dchain_rejuvenate_index(dynamic_ft->dyn_heap, index, time);

    struct DynamicValue* value = 0;
    vector_borrow(dynamic_ft->dyn_vals, index, (void**)&value);

    assert(0 <= time);
    uint64_t time_u = (uint64_t) time;
    assert(sizeof(vigor_time_t) == sizeof(int64_t));
    assert(value->bucket_time >= 0);
    assert(value->bucket_time <= time_u);
    uint64_t time_diff = time_u - value->bucket_time;
    if (time_diff < config.burst * VIGOR_TIME_SECONDS_MULTIPLIER / config.rate) {
      uint64_t added_tokens = time_diff * config.rate / VIGOR_TIME_SECONDS_MULTIPLIER;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"
      vigor_note(0 <= time_diff * config.rate / VIGOR_TIME_SECONDS_MULTIPLIER);
#pragma GCC diagnostic pop
      assert(value->bucket_size <= config.burst);
      value->bucket_size += added_tokens;
      if (value->bucket_size > config.burst) {
        value->bucket_size = config.burst;
      }
    } else {
      value->bucket_size = config.burst;
    }
    value->bucket_time = time_u;

    bool fwd = false;
    if (value->bucket_size > size) {
      value->bucket_size -= size;
      fwd = true;
    }

    vector_return(dynamic_ft->dyn_vals, index, value);

    return fwd;
  } else {
    if (size > config.burst) {
      NF_DEBUG("  Unknown flow with packet larger than burst size. Dropping.");
      return false;
    }

    int allocated = dchain_allocate_new_index(dynamic_ft->dyn_heap,
                                              &index,
                                              time);
    if (!allocated) {
      NF_DEBUG("No more space in the policer table");
      return false;
    }
    uint32_t *key;
    struct DynamicValue* value = 0;
    vector_borrow(dynamic_ft->dyn_keys, index, (void**)&key);
    vector_borrow(dynamic_ft->dyn_vals, index, (void**)&value);
    *key = dst;
    value->bucket_size = config.burst - size;
    value->bucket_time = time;
    map_put(dynamic_ft->dyn_map, key, index);
    //the other half of the key is in the map
    vector_return(dynamic_ft->dyn_keys, index, key);
    vector_return(dynamic_ft->dyn_vals, index, value);

    NF_DEBUG("  New flow. Forwarding.");
    return true;
  }
}

void nf_core_init(void) {
  unsigned capacity = config.dyn_capacity;
  dynamic_ft = alloc_state(capacity, rte_eth_dev_count());
  if (dynamic_ft == NULL) {
    rte_exit(EXIT_FAILURE, "error allocating nf state");
  }
}

int nf_core_process(struct rte_mbuf* mbuf, vigor_time_t now) {
  NF_DEBUG("Received packet");
  const uint16_t in_port = mbuf->port;
  struct ether_hdr* ether_header = nf_then_get_ether_header(mbuf->buf_addr);

  if (!RTE_ETH_IS_IPV4_HDR(mbuf->packet_type) &&
      !(mbuf->packet_type == 0 &&
        ether_header->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4))) {
    NF_DEBUG("Not IPv4, dropping");
    return in_port;
  }

  uint8_t* ip_options;
  bool wellformed = true;
	struct ipv4_hdr* ipv4_header = nf_then_get_ipv4_header(mbuf->buf_addr, &ip_options, &wellformed);
  if (!wellformed) {
		NF_DEBUG("Malformed IPv4, dropping");
    return in_port;
  }
  assert(ipv4_header != NULL);

  policer_expire_entries(now);

  if (in_port == config.lan_device) {
    // Simply forward outgoing packets.
    NF_DEBUG("Outgoing packet. Not policing.");
    return config.wan_device;
  } else if (in_port == config.wan_device) {
    // Police incoming packets.
    bool fwd = policer_check_tb(ipv4_header->dst_addr, mbuf->pkt_len, now);

    if (fwd) {
      NF_DEBUG("Incoming packet within policed rate. Forwarding.");
      return config.lan_device;
    } else {
      NF_DEBUG("Incoming packet outside of policed rate. Dropping.");
      return config.wan_device;
    }
  } else {
    // Drop any other packets.
    NF_DEBUG("Unknown port. Dropping.");
    return in_port;
  }
}

void nf_config_init(int argc, char** argv) {
  policer_config_init(&config, argc, argv);
}

void nf_config_cmdline_print_usage(void) {
  policer_config_cmdline_print_usage();
}

void nf_print_config() {
  policer_print_config(&config);
}
