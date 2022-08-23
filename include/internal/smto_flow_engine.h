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

#ifndef SMART_OFFLOAD_SRC_SMTO_FLOW_ENGINE_H_
#define SMART_OFFLOAD_SRC_SMTO_FLOW_ENGINE_H_

#include <stdint.h>
#include "smto.h"
#include "internal/smto_flow_key.h"

/**
 * Create a default jump rule which make pkts jump from group 0 to 1.
 *
 * @param port_id The port which the flow will be affect.
 *
 * @return
 *      - Not NULL: Create success.
 *      - NULL: Some error occur when create a rte_flow.
 */
struct rte_flow *create_default_jump_flow(uint16_t port_id);


/**
* Create a default rss rule which can match all packet.
*
* @param smto_cb The control block of SmartOffload.
* @param port_id The port which the flow will be affect.
 *
* @return
*      - Not NULL: Create success.
*      - NULL: Some error occur when create a rte_flow.
*/
struct rte_flow *create_default_rss_flow(uint16_t port_id);

/**
 * Create a offload flow which match by a ipv4 5-tuple.
 *
 * @param port_id The port which the flow will be affect.
 * @param flow_key The ipv4 5-tuple which used to match packet.
 * @param error The error return by creating rte_flow.
 * @return
*      - not NULL: Create success.
*      - NULL: Some error occur when create a rte_flow.
 */
struct rte_flow *create_general_offload_flow(uint16_t port_id,
                                             struct smto_flow_key *flow_key,
                                             struct rte_flow_error *error);

/**
 * A worker to pull rules from packet workers and create rte_flow.
 *
 * @param args worker_parameter
 * @return
 */
int create_flow_loop(void *args);

#endif //SMART_OFFLOAD_SRC_SMTO_FLOW_ENGINE_H_
