from state import flow_emap, int_devices
EXP_TIME = 10 * 1000
EXT_DEVICE = 1

if a_packet_received:
    flow_emap.expire_all(now - EXP_TIME)

h3 = pop_header(tcpudp, on_mismatch=([],[]))
h2 = pop_header(ipv4, on_mismatch=([],[]))
h1 = pop_header(ether, on_mismatch=([],[]))

internal_flow = FlowIdc(h3.src_port, h3.dst_port, h2.saddr, h2.daddr, h2.npid)
if (received_on_port != EXT_DEVICE and
    not flow_emap.has(internal_flow) and
    not flow_emap.full()) :
    fl_id = the_index_allocated
    flow_emap.add(internal_flow, fl_id, now)
    vector_set(int_devices, fl_id, received_on_port)
else:
    pass
