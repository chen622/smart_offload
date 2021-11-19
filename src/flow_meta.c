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
#include "flow_meta.h"


struct flow_meta *create_flow_meta(uint32_t pkt_len) {
    struct flow_meta *data = rte_zmalloc("flow_table_data", sizeof(struct flow_meta), 0);
    data->create_at = rte_rdtsc(); // The number of cycles of CPU. One cycle is 1/rte_get_tsc_hz() second.
    data->flow_size = pkt_len;
    data->packet_amount = 1;
    data->flow = NULL;
    data->is_offload = false;
    return data;
}

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

void dump_pkt_info(union ipv4_5tuple_host *key, uint16_t qi, char *result, int result_length) {
    char pkt_info[2][100] = {{0},
                             {0},};

    uint32_t src_ip = rte_be_to_cpu_32(key->ip_src);
    format_ipv4_addr("src_ip=", src_ip, pkt_info[0]);
    uint32_t dst_ip = rte_be_to_cpu_32(key->ip_dst);
    format_ipv4_addr(" - dst_ip=", dst_ip, pkt_info[1]);
    uint16_t src_port = rte_be_to_cpu_16(key->port_src);
    uint16_t dst_port = rte_be_to_cpu_16(key->port_dst);

    snprintf(result, result_length, "%s:%u%s:%u - queue=0x%x", pkt_info[0], src_port, pkt_info[1], dst_port,
             (unsigned int) qi);
}
