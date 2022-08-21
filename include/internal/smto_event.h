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

#ifndef SMART_OFFLOAD_SRC_SMTO_EVENT_H_
#define SMART_OFFLOAD_SRC_SMTO_EVENT_H_

#include <rte_alarm.h>
#include <stdint.h>
#include "smto.h"


#define TIMEOUT_FLOW_BATCH_SIZE 8

/**
 * Used to queue the result of a flow counter.
 *
 * @param port_id The port id of flow.
 * @param flow The target flow.
 * @param query_counter The result.
 * @param error The error occur when querying a flow.
 * @return 0 on success, other on error.
 */
int query_counter(uint16_t port_id, struct rte_flow *flow, struct rte_flow_query_count *counter,
                  struct rte_flow_error *error);

/**
 * Register a callback function to delete the flow which has timeout.
 *
 * @param port_id The port id of flow.
 *
 * @return 0 on success, other on error.
 */
int register_aged_event(uint16_t port_id);

/**
 * Unregister the callback funtion to delete the flow which has timeout.
 *
 * @param port_id The port id of flow.
 *
 * @return 0 on success, other on error.
 */
int unregister_aged_event(uint16_t port_id);

#endif //SMART_OFFLOAD_SRC_SMTO_EVENT_H_
