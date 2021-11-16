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

#ifndef SMART_OFFLOAD_FLOW_MANAGEMENT_H
#define SMART_OFFLOAD_FLOW_MANAGEMENT_H

#include <rte_flow.h>
#include <rte_ethdev.h>
#include <zlog.h>
#include "hash_key.h"


struct rte_flow *create_offload_rte_flow(uint16_t port_id, union ipv4_5tuple_host *flow_key, zlog_category_t *zc,
                                         struct rte_flow_error *error);

#endif //SMART_OFFLOAD_FLOW_MANAGEMENT_H
