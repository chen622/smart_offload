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

#ifndef SMART_OFFLOAD_FLOW_META_H
#define SMART_OFFLOAD_FLOW_META_H

#include <rte_flow.h>

/**
 * The data structure to save statistical information of flow.
 */
struct flow_meta {
    /* Use the number of cycles of CPU as the time*/
    uint64_t create_at;
    /* Total size of flow */
    uint32_t flow_size;
    /* Total amount of packets in a flow */
    uint32_t packet_amount;
    /* Has created rte_flow to offload flow or not */
    bool is_offload;
    /* The rte_flow to handle this flow */
    struct rte_flow *flow;
};

struct flow_meta *create_flow_meta(uint32_t pkt_len);

#endif //SMART_OFFLOAD_FLOW_META_H
