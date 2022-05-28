/*
 * MIT License
 * 
 * Copyright (c) 2021 Chenming C (ccm@ccm.ink)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include <rte_malloc.h>
#include <sys/time.h>
#include "flow_management.h"

static __thread zlog_category_t *zc = 0;
static rte_xmm_t mask0;

volatile bool force_quit = false;

/**
 * Extract ipv4 5 tuple from the mbuf by SSE3 instruction.
 *
 * @param m0 The mbuf memory.
 * @param mask0 The position which doesn't need.
 * @param key The result.
 */
static __rte_always_inline void get_ipv4_5tuple(struct rte_mbuf *m0, __m128i mask0, union ipv4_5tuple_host *key) {
    __m128i tmpdata0 = _mm_loadu_si128(
            rte_pktmbuf_mtod_offset(m0, __m128i *,
                                    sizeof(struct rte_ether_hdr) +
                                    offsetof(struct rte_ipv4_hdr, time_to_live)));

    key->xmm = _mm_and_si128(tmpdata0, mask0);
}

/**
 * Check the packet type and process it with corresponding method.
 *
 * @param m The packet raw data.
 * @param queue_index The index of queue which the packet comes from.
 * @param key The key used to hash.
 * @return Success or not:
 *     - 0: success.
 *     - 1: unsupported return value of hash map lookup.
 *     - -1: cannot add key into hash map.
 *     - -2: create offload rte_flow failed.
 */
static inline int
packet_processing(struct rte_hash *flow_hash_map, struct rte_mbuf *m, uint16_t queue_index, uint16_t port_id) {
    union ipv4_5tuple_host *flow_map_key = 0;
    uint64_t start_tsc = rte_rdtsc();
    int ret = 0;
    flow_map_key = rte_zmalloc("flow_key_context", sizeof(xmm_t), 0);
    zlog_debug(zc, "flow_key rte_create use %f ns", GET_NANOSECOND(start_tsc));
//    rte_free(flow_map_key);
//    start_tsc = rte_rdtsc();
//    flow_map_key = malloc(sizeof(xmm_t));
//    memset(flow_map_key, 0 , sizeof(xmm_t));
//    free(flow_map_key);
//    zlog_debug(zc, "flow_key create use %f ns", GET_NANOSECOND(start_tsc));
    /* Used to extract useful variables from memory */
    mask0 = (rte_xmm_t) {.u32 = {BIT_8_TO_15, ALL_32_BITS,
                                 ALL_32_BITS, ALL_32_BITS}};

    if (m->packet_type & RTE_PTYPE_L3_IPV4 && (m->packet_type & (RTE_PTYPE_L4_UDP | RTE_PTYPE_L4_TCP))) {

        start_tsc = rte_rdtsc();
        get_ipv4_5tuple(m, mask0.x, flow_map_key);
        zlog_debug(zc, "get ip header use %f ns", GET_NANOSECOND(start_tsc));

        start_tsc = rte_rdtsc();
        char pkt_info[MAX_ERROR_MESSAGE_LENGTH];
        dump_pkt_info(flow_map_key, queue_index, pkt_info, MAX_ERROR_MESSAGE_LENGTH);
        zlog_debug(zc, "dump pkt use %f ns", GET_NANOSECOND(start_tsc));

        start_tsc = rte_rdtsc();
        struct flow_meta *flow_map_data = 0;
        ret = rte_hash_lookup_data(flow_hash_map, flow_map_key, (void **) &flow_map_data);
        zlog_debug(zc, "hash lookup use %f ns", GET_NANOSECOND(start_tsc));

//
//        zlog_debug(zc, "packet(%u)(%s)(%d): %s", m->pkt_len, m->packet_type & RTE_PTYPE_L4_UDP ? "UDP" : "TCP", ret,
//                   pkt_info);

        if (ret == -ENOENT) { // A flow that has not yet appeared
            flow_map_data = create_flow_meta(m->pkt_len);
            start_tsc = rte_rdtsc();
            ret = rte_hash_add_key_data(flow_hash_map, flow_map_key, flow_map_data);
            zlog_debug(zc, "add hash map use %f ns", GET_NANOSECOND(start_tsc));
            if (ret != 0) {
                zlog_error(zc, "cannot add pkt:%s into flow table, return error %d", pkt_info, ret);
                return -1;
            } else {
//                zlog_debug(zc, "first packet processing delay: %f ns",
//                           GET_NANOSECOND(start_tsc));
                zlog_debug(zc, "success add a flow(%s) to flow hash table", pkt_info);
                return 0;
            }
        } else if (ret >= 0) { // A flow that has appeared before
            flow_map_data->packet_amount++;
            flow_map_data->flow_size += m->pkt_len;
//            zlog_debug(zc,
//                       "capture a packet which belong to a flow in flow table, which already have %u packets and total size is %u",
//                       flow_map_data->packet_amount, flow_map_data->flow_size);

            /* Assume the flow can be offloaded now */
            if (flow_map_data->packet_amount == 5) {
                struct rte_flow_error flow_error = {0};

                start_tsc = rte_rdtsc();
                struct rte_flow *flow = create_offload_rte_flow(port_id, flow_hash_map, flow_map_key, zc,
                                                                &flow_error);
                zlog_debug(zc, "create and apply rte_flow use %f ns", GET_NANOSECOND(start_tsc));

                if (flow == NULL) {
                    zlog_error(zc, "cannot create a offload flow of packet(%s): %s", pkt_info, flow_error.message);
                    return -2;
                }
                zlog_info(zc, "a flow(%s) has been offload to network card", pkt_info);
                flow_map_data->is_offload = true;
                flow_map_data->flow = flow;
//                zlog_debug(zc, "fifth packet processing delay: %f ns",
//                           GET_NANOSECOND(start_tsc));
                return 0;
            }
            return 0;
        } else {
            zlog_error(zc, "unhandled error occur");
            return 1;
        }
    }
}


int process_loop(void *args) {
    unsigned lcore_id;
    zc = zlog_get_category("worker");
    lcore_id = rte_lcore_id();
    uint16_t queue_id = ((struct worker_parameter *) args)->queue_id;
    uint16_t port_id = ((struct worker_parameter *) args)->port_id;
    struct rte_hash *flow_hash_map = ((struct worker_parameter *) args)->flow_hash_map;
    zlog_info(zc, "worker #%u for queue #%u start working!", lcore_id, queue_id);

    /* Pre-allocate the local variable */
    struct rte_mbuf *mbufs[32] = {0};
    uint16_t nb_rx;
    uint16_t nb_tx;
    uint16_t packet_index;
    /* pull packet from queue and process */
    while (!force_quit) {
        nb_rx = rte_eth_rx_burst(port_id, queue_id, mbufs, 32);
        if (nb_rx) {
            for (packet_index = 0; packet_index < nb_rx; packet_index++) {
                struct rte_mbuf *m = mbufs[packet_index];
                packet_processing(flow_hash_map, m, queue_id, port_id);
            }
            nb_tx = rte_eth_tx_burst(port_id, queue_id,
                                     mbufs, nb_rx);
        }
        /* Free any unsent packets. */
        if (unlikely(nb_tx < nb_rx)) {
            uint16_t buf;
            for (buf = nb_tx; buf < nb_rx; buf++)
                rte_pktmbuf_free(mbufs[buf]);
        }
    }

    return 0;
}
