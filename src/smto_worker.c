/*
 * MIT License
 * 
 * Copyright (c) 2022 Chenming C (ccm@ccm.ink)
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

#include "smto.h"
#include "internal/smto_worker.h"
#include "internal/smto_flow_key.h"
#include "internal/smto_flow_engine.h"

extern struct smto *smto_cb;

static rte_xmm_t ipv4_mask = (rte_xmm_t) {
    .u32 = {BIT_8_TO_15, ALL_32_BITS,
            ALL_32_BITS, ALL_32_BITS}};

/**
 * Extract ipv4 5 tuple from the mbuf by SSE3 instruction.
 *
 * @param m0 The mbuf memory.
 * @param mask0 The position which doesn't need.
 * @param key The result.
 */
static __rte_always_inline void get_ipv4_5tuple(struct rte_mbuf *m0, __m128i mask0, struct smto_flow_key *key) {
  __m128i tmpdata0 = _mm_loadu_si128(
      rte_pktmbuf_mtod_offset(m0, __m128i *,
                              sizeof(struct rte_ether_hdr) +
                                  offsetof(struct rte_ipv4_hdr, time_to_live)));

  key->xmm = _mm_and_si128(tmpdata0, mask0);
  key->tuple.proto = key->proto;
}

static __rte_always_inline int packet_processing(struct rte_mbuf *pkt_mbuf, uint16_t queue_index, uint16_t port_id) {
  int ret = 0;
  struct smto_flow_key *flow_key = rte_zmalloc("flow_key", sizeof(struct smto_flow_key), 0);
//  zlog_debug(smto_cb->logger, "flow_key: %u", pkt_mbuf->packet_type);
  if (pkt_mbuf->packet_type & RTE_PTYPE_L3_IPV4 && (pkt_mbuf->packet_type & (RTE_PTYPE_L4_UDP | RTE_PTYPE_L4_TCP))) {
    get_ipv4_5tuple(pkt_mbuf, ipv4_mask.x, flow_key);

    char pkt_info[MAX_PKT_INFO_LENGTH];
#ifndef RELEASE
    dump_pkt_info(&flow_key->tuple, -2, pkt_info, MAX_PKT_INFO_LENGTH);
#endif
    struct smto_flow_key *old_flow_key = 0;
    ret = rte_hash_lookup_data(smto_cb->flow_hash_map, &flow_key->tuple, (void **) &old_flow_key);
    if (ret == -ENOENT) { ///< A flow that has not appeared
      flow_key->create_at = rte_rdtsc();
      flow_key->packet_amount++;
      flow_key->flow_size += pkt_mbuf->pkt_len;
      ret = rte_hash_add_key_data(smto_cb->flow_hash_map, &flow_key->tuple, flow_key);
      if (ret != 0) {
        zlog_error(smto_cb->logger, "cannot add pkt(%s) into flow table: %s", pkt_info, rte_strerror(ret));
        return SMTO_ERROR_HASH_MAP_OPERATION;
      } else {
        zlog_debug(smto_cb->logger, "success add a flow(%s) to flow hash table", pkt_info);
        return SMTO_SUCCESS;
      }
    } else if (ret >= 0) {
      rte_free(flow_key);
      flow_key = old_flow_key;
      flow_key->packet_amount++;
      flow_key->flow_size += pkt_mbuf->pkt_len;
      zlog_debug(smto_cb->logger,
                 "capture a packet which belong to a flow in flow table, which already have %u packets and total size is %u",
                 flow_key->packet_amount,
                 flow_key->flow_size);

      /* Assume the flow can be offloaded now */
      if (flow_key->packet_amount == PKT_AMOUNT_TO_OFFLOAD) {
        struct rte_flow_error flow_error = {0};

        struct rte_flow *flow = create_general_offload_flow(port_id, flow_key, &flow_error);

        if (flow == NULL) {
          zlog_error(smto_cb->logger, "cannot create a offload flow of packet(%s): %s", pkt_info, flow_error.message);
          return SMTO_ERROR_FLOW_CREATE;
        }
        zlog_info(smto_cb->logger, "a flow(%s) has been offload to network card", pkt_info);
        flow_key->is_offload = true;
        flow_key->flow = flow;
//                zlog_debug(zc, "fifth packet processing delay: %f ns",
//                           GET_NANOSECOND(start_tsc));
      }
      return SMTO_SUCCESS;
    }
    return ret;
  } else {
    zlog_error(smto_cb->logger, "Packet type is not supported.");
    return SMTO_ERROR_UNSUPPORTED_PACKET_TYPE;
  }
}

int process_loop(void *args) {
  unsigned lcore_id;
  lcore_id = rte_lcore_id();
  uint16_t queue_id = ((struct worker_parameter *) args)->queue_id;
  uint16_t port_id = ((struct worker_parameter *) args)->port_id;
  zlog_info(smto_cb->logger, "worker #%u for queue #%u start working!", lcore_id, queue_id);

  /// Pre-allocate the local variable
  struct rte_mbuf *mbufs[MAX_BULK_SIZE] = {0};
  uint16_t nb_rx;
  uint16_t nb_tx;
  uint16_t packet_index;

  /// Pull packet from queue and process
  while (smto_cb->is_running) {
    nb_rx = rte_eth_rx_burst(port_id, queue_id, mbufs, MAX_BULK_SIZE);
    if (nb_rx) {
      for (packet_index = 0; packet_index < nb_rx; packet_index++) {
        struct rte_mbuf *pkt_mbuf = mbufs[packet_index];
        packet_processing(pkt_mbuf, queue_id, port_id);
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
  zlog_info(smto_cb->logger, "worker #%u for queue #%u stop working!", lcore_id, queue_id);

  return 0;
}