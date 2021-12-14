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
#include <rte_hash.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <zlog.h>
#include "hash_key.h"
#include "smart_offload.h"

/**
 * Used to init a schedule plan to select queue by rss.
 */
static uint16_t queue_schedule[] = {3, 1, 2, 4, 5, 7, 0, 6};
//static uint16_t queue_schedule[] = {0};

/**
 * Create a default jump rule which make pkts jump from group 0 to 1.
 *
 * @param port_id The port which the flow will be affect.
 * @param error The error return by creating rte_flow.
 * @return
 *      - not NULL: Create success.
 *      - NULL: Some error occur when create a rte_flow.
 */
struct rte_flow *create_default_jump_flow(uint16_t port_id, struct rte_flow_error *error);

/**
 * Create a default rss rule which can match all packet.
 *
 * @param port_id The port which the flow will be affect.
 * @param queue_amount The amount of queues which can be used to rss.
 * @param error The error return by creating rte_flow.
 * @return
 *      - not NULL: Create success.
 *      - NULL: Some error occur when create a rte_flow.
 */
struct rte_flow *create_default_rss_flow(uint16_t port_id, uint16_t queue_amount, struct rte_flow_error *error);

/**
 * Create a offload flow which match by a ipv4 5-tuple.
 *
 * @param port_id The port which the flow will be affect.
 * @param flow_key The ipv4 5-tuple which used to match packet.
 * @param zc The descriptor used to write logs.
 * @param error The error return by creating rte_flow.
 * @return
*      - not NULL: Create success.
*      - NULL: Some error occur when create a rte_flow.
 */
struct rte_flow *
create_offload_rte_flow(uint16_t port_id, struct rte_hash *flow_hash_table, union ipv4_5tuple_host *flow_key,
                        zlog_category_t *zc,
                        struct rte_flow_error *error);

#endif //SMART_OFFLOAD_FLOW_MANAGEMENT_H
