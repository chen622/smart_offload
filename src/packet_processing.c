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

#include "smart_offload.h"

__thread zlog_category_t *zc;

static inline void print_ether_addr(const char *ex, struct rte_ether_addr *eth_addr, char *result) {
    char buf[RTE_ETHER_ADDR_FMT_SIZE];
    rte_ether_format_addr(buf, RTE_ETHER_ADDR_FMT_SIZE, eth_addr);
    sprintf(result, "%s%s", ex, buf);
}

static inline void format_ipv4_addr(const char *ex, const uint32_t ip, char *result) {
    sprintf(result, "%s%d.%d.%d.%d", ex,
            (ip >> 24) & 0x000000ff,
            (ip >> 16) & 0x000000ff,
            (ip >> 8) & 0x000000ff,
            (ip) & 0x000000ff);
}

static inline void dump_pkt_info(struct rte_mbuf *m, uint16_t qi) {
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ip_hdr;
    char pkt_info[4][100] = {{0},
                             {0},
                             {0},
                             {0}};

    eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
    print_ether_addr("src_mac=", &eth_hdr->s_addr, pkt_info[0]);
    print_ether_addr(" - dst_mac=", &eth_hdr->d_addr, pkt_info[1]);
    rte_pktmbuf_adj(m, (uint16_t) sizeof(struct rte_ether_hdr));
    ip_hdr = rte_pktmbuf_mtod(m, struct rte_ipv4_hdr *);
    uint32_t src_ip = rte_be_to_cpu_32(ip_hdr->src_addr);
    format_ipv4_addr(" - src_ip=", src_ip, pkt_info[2]);
    uint32_t dst_ip = rte_be_to_cpu_32(ip_hdr->dst_addr);
    format_ipv4_addr(" - dst_ip=", dst_ip, pkt_info[3]);
    zlog_debug(zc, "%s%s%s%s - queue=0x%x", pkt_info[0], pkt_info[1], pkt_info[2], pkt_info[3],
               (unsigned int) qi);
}

int packet_processing(void *args) {
    unsigned lcore_id;
    zc = zlog_get_category("worker");
    lcore_id = rte_lcore_id();
    zlog_info(zc, "worker%u start working!", lcore_id);

    struct rte_mbuf *mbufs[32];
    struct rte_flow_error error;
    /* pre-allocate the local variable */
    uint16_t port_id;
    uint16_t nb_rx;
    uint16_t nb_tx;
    uint16_t i;
    uint16_t j;

    while (!force_quit) {
        for (i = 0; i < GENERAL_QUEUES_QUANTITY; i++) {
            nb_rx = rte_eth_rx_burst(port_id,
                                     i, mbufs, 32);
            if (nb_rx) {
                for (j = 0; j < nb_rx; j++) {
                    struct rte_mbuf *m = mbufs[j];

                    dump_pkt_info(m, i);
                }
                nb_tx = rte_eth_tx_burst(port_id, i,
                                         mbufs, nb_rx);
            }
            /* Free any unsent packets. */
            if (unlikely(nb_tx < nb_rx)) {
                uint16_t buf;
                for (buf = nb_tx; buf < nb_rx; buf++)
                    rte_pktmbuf_free(mbufs[buf]);
            }
        }
    }

    return 0;
}