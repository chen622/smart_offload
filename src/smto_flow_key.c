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


#include <rte_byteorder.h>
#include "internal/smto_flow_key.h"

void dump_pkt_info(struct rdarm_five_tuple *key, int qi, char *result, int result_length) {
  uint32_t src_ip = rte_be_to_cpu_32(key->ip1);
  uint32_t dst_ip = rte_be_to_cpu_32(key->ip2);
  uint16_t src_port = rte_be_to_cpu_16(key->port1);
  uint16_t dst_port = rte_be_to_cpu_16(key->port2);
  uint8_t proto = key->proto;

  if (qi >= 0) {
    snprintf(result, result_length, "%d.%d.%d.%d:%u-(%u)-%d.%d.%d.%d:%u - queue=0x%x",
             (src_ip >> 24) & 0x000000ff,
             (src_ip >> 16) & 0x000000ff,
             (src_ip >> 8) & 0x000000ff,
             (src_ip) & 0x000000ff, src_port, proto,
             (dst_ip >> 24) & 0x000000ff,
             (dst_ip >> 16) & 0x000000ff,
             (dst_ip >> 8) & 0x000000ff,
             (dst_ip) & 0x000000ff, dst_port,
             (int) qi);
  } else {
    snprintf(result, result_length, "%d.%d.%d.%d:%u-(%u)-%d.%d.%d.%d:%u",
             (src_ip >> 24) & 0x000000ff,
             (src_ip >> 16) & 0x000000ff,
             (src_ip >> 8) & 0x000000ff,
             (src_ip) & 0x000000ff, src_port, proto,
             (dst_ip >> 24) & 0x000000ff,
             (dst_ip >> 16) & 0x000000ff,
             (dst_ip >> 8) & 0x000000ff,
             (dst_ip) & 0x000000ff, dst_port);
  }
}
