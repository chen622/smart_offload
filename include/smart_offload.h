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

#ifndef SMART_OFFLOAD_SMART_OFFLOAD_H
#define SMART_OFFLOAD_SMART_OFFLOAD_H

#include <stdio.h>
#include <signal.h>
#include <zlog.h>
#include <rte_ethdev.h>

#define CHECK_INTERVAL 1000  /* 100ms */
#define MAX_REPEAT_TIMES 90  /* 9s (90 * 100ms) in total */

/**
 * The quantity of different queues.
 */
#define GENERAL_QUEUES_QUANTITY 8
#define HAIRPIN_QUEUES_QUANTITY 1

/**
 * Shutdown flag
 */
volatile bool force_quit;



/**
 * Shutdown event has been triggered.
 *
 * @param exit_code
 *   Is shutdown by a failure or not.
 *   - EXIT_SUCCESS	0
 *   - EXIT_FAILURE	1
 * @param format
 *   The message to describe the event.
 */
void smto_exit(int exit_code, const char *format);

/**
 * Configure a network port and initialize the rx/tx queues.
 *
 * @param port_id
 *   The ID of port which will be configured.
 * @param mbuf_pool
 *   The mempool used to initialize queues.
 */
void init_port(int port_id, struct rte_mempool *mbuf_pool);


/**
 * Setup the hairpin configure on peer ports:
 *   P0 <-> P1, P2 <-> P3, etc
 */
void setup_hairpin();

/**
 * Check the status of the network port.
 *
 * @param port_id
 *   The port will be checked.
 */
void assert_link_status(uint16_t port_id);

/**
 * Run on the worker core to process packets from rx queues.
 *
 */
int packet_processing(void *args);

#endif //SMART_OFFLOAD_SMART_OFFLOAD_H
