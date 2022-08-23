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

#ifndef SMART_OFFLOAD_SRC_SMTO_PORT_H_
#define SMART_OFFLOAD_SRC_SMTO_PORT_H_

#include <rte_mempool.h>
#include "smto.h"

#define CHECK_INTERVAL 1000 ///< 100ms
#define MAX_REPEAT_TIMES 90 ///< waiting for 9s (90 * 100ms) in total

/**
 * Check the link status of port.
 *
 * @param port_id The port to be checked.
 *
 * @return 0 on success, other on error.
 */
int assert_link_status(uint16_t port_id);

/**
 * Configure a network port and initialize the rx/tx queues.
 *
 * @param port_id The ID of port which will be configured.
 *
 * @return 0 on success, other on error.
 */
int init_port(uint16_t port_id);

/**
 * Setup the one-port mode hairpin.
 *
 * @param port_id The port to be setup.
 *
 * @return 0 on success, other on error.
 */
int setup_one_port_hairpin(int port_id);

/**
 * Free the memory of flows in the hash map and free the flow hash map.
 *
 * @return 0 on success, other on error.
 */
int destroy_hash_map();

#endif //SMART_OFFLOAD_SRC_SMTO_PORT_H_
