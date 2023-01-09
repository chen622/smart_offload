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

#ifndef SMART_OFFLOAD_INCLUDE_SMTO_H_
#define SMART_OFFLOAD_INCLUDE_SMTO_H_

#include <zlog.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <rte_malloc.h>
#include <rte_ethdev.h>
#include <rte_hash.h>
#include <rte_ring.h>
#include <rdarm.h>

#include "smto_comon.h"
#include "internal/smto_worker.h"

/// The number of elements in the mbuf pool. The optimum size is (2^q - 1). Each mbuf is 2176 bytes.
#define NUM_MBUFS 1048575

/// Size of the per-core object cache. Must lower or equal to RTE_MEMPOOL_CACHE_MAX_SIZE and n / 1.5
#define CACHE_SIZE 512

/// The quantity of rx/tx queues.
#define GENERAL_QUEUES_QUANTITY 1

/// The index of hairpin queue.
#define HAIRPIN_QUEUE_INDEX GENERAL_QUEUES_QUANTITY

/// The packet descriptor of each queue.
#define QUEUE_DESC_NUMBER 128

/// The max flow key of the hash flow table.
#define MAX_HASH_ENTRIES (1024 * 1024 * 32)

/// The max bulk amount to pull from queue.
#define MAX_BULK_SIZE 32

/// The max amount of ring to transfer flow rules.
#define MAX_RING_ENTRIES (1024*16)

/// The amount of packets to create a flow rule.
#define PKT_AMOUNT_TO_OFFLOAD 5

/// The seconds to timeout
#define FLOW_TIMEOUT_SECOND 10

extern const uint32_t SRC_IP;

/// The main control block of SmartOffload.
struct smto {
  volatile bool is_running;  ///< Whether the SmartOffload is running.
  zlog_category_t *logger;
  enum smto_mode mode;
  uint16_t ports[2];
  struct rte_mempool *pkt_mbuf_pool;
  struct rte_hash *flow_hash_map;
  struct rte_ring *flow_rules_ring;
  struct rte_ring *port_pool;
};


/**
 * Initialize SmartOffload framework.
 * @param smto_cb The main control block of SmartOffload.
 *
 * @return The result of initialization. Get the error msg by calling smto_error_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int init_smto(struct smto **smto_cb);

/**
 * Destroy SmartOffload.
 * @param smto_cb The main control block of SmartOffload.
 */
void destroy_smto(struct smto *smto_cb);

#endif //SMART_OFFLOAD_INCLUDE_SMTO_H_
