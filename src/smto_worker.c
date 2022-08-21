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

extern struct smto *smto_cb;

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
        struct rte_mbuf *m = mbufs[packet_index];
//        packet_processing(flow_hash_map, m, queue_id, port_id);
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