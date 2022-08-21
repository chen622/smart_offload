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

#ifndef SMART_OFFLOAD_FLOW_EVENT_H
#define SMART_OFFLOAD_FLOW_EVENT_H

#include <stdint.h>
#include <rte_flow.h>
#include <rte_ethdev.h>
#include <rte_hash.h>
#include <rte_alarm.h>
#include <rte_malloc.h>
#include <zlog.h>
#include "hash_key.h"
#include "smart_offload.h"
#include "flow_meta.h"

/**
 * Used to pass port id and flow map to the function which delete the timeout flow.
 */
struct delete_flow_params {
    uint16_t port_id;
    struct rte_hash *flow_hash_map;
};

/**
 * Register a callback function to delete the flow which has timeout.
 *
 * @param port_id The port id of flow.
 * @param flow_hash_map Used to pass flow map to the callback function.
 * @return
 *      - On success, zero.
 *      - On failure, a negative value.
 */
int register_aged_event(uint16_t port_id, struct rte_hash *flow_hash_map);

#endif //SMART_OFFLOAD_FLOW_EVENT_H
