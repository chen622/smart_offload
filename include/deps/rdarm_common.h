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

#ifndef RDARM_INCLUDE_RDARM_COMMON_H_
#define RDARM_INCLUDE_RDARM_COMMON_H_

#include <zlog.h>
#include <semaphore.h>
#include <malloc.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <rdma/rdma_cma.h>

/**
 * @brief Max amount of nodes.
 */
#define RDARM_NODES_NUM 4
/**
 * @brief Server port.
 */
#define RDARM_PORT 7174
/**
 * @brief The amount of slots.
 */
#define RDARM_SLOT_NUM  1024
/**
 * @brief The max byte length of key.
 */
#define RDARM_KEY_LEN 16

/**
 * @brief The quantity of hash table in each slot.
 */
#define RDARM_HASH_TABLE_NUM 8

/**
 * @brief The quantity of entries in each hash table.
 */
#define RDARM_BUCKET_NUM 5000

/**
 * @brief The quantity of entries to save the collision value.
 */
#define RDARM_ADDITION_BUCKET_NUM 500

/**
 * @brief The seed used to generate hash value for slot.
 */
#define SLOT_KEY_SEED 6220

#define NODE_HASH_SEED 6220

/**
 * @brief The seed used to generate hash value for entry.
 */
#define HASHMAP_KEY_SEED 5210

#define RDARM_QP_DEPTH 20

#define RDARM_RESOLVED_ADDRESS_TIMEOUT_MS 2000

/**
 * @brief The amount of request buffers. Should be a even number and less or equal 2^8=255.
 */
#define RDARM_CONCURRENCY_WRITE 200

/**
 * @brief The size of each request buffer.
 */
#define RDARM_COMPLEX_COMMUNICATE_DATA_SIZE 100

/**
 * @brief The five tuple to save sip, dip, sport, dport, proto, and a prefix.
 */
typedef struct rdarm_five_tuple {
  uint8_t proto;
  char prefix[3];
  uint32_t ip1;
  uint32_t ip2;
  uint16_t port1;
  uint16_t port2;
} rdarm_five_tuple;

/**
 * @brief The error code of initialization or destruction.
 */
enum rdarm_error_code {
  RDARM_SUCCESS = 0,
  RDARM_ERROR_INVALID_PARAMETER,
  RDARM_ERROR_FAILED_TO_ALLOC_MEMORY,
  RDARM_ERROR_FAILED_TO_CREATE_THREAD,
  RDARM_ERROR_FAILED_TO_CREATE_CO,
  RDARM_ERROR_FAILED_TO_CREATE_SHARE_STACK,
  RDARM_ERROR_FAILED_TO_CREATE_SPIN_LOCK,
  RDARM_ERROR_FAILED_TO_SPIN_LOCK,
  RDARM_ERROR_FAILED_TO_CREATE_SEMAPHORE,
  RDARM_ERROR_FAILED_TO_CHANGE_SEMAPHORE,

  RDARM_ERROR_INVALID_IP_ADDRESS,
  RDARM_ERROR_NO_IB_DEVICE,
  RDARM_ERROR_CM_EVENT_CHANNEL,
  RDARM_ERROR_CM_ID,
  RDARM_ERROR_PROTECTION_DOMAIN,
  RDARM_ERROR_CM_BIND_ADDRESS,
  RDARM_ERROR_CM_LISTEN,
  RDARM_ERROR_CM_THREAD,
  RDARM_ERROR_QUEUE_PAIR,
  RDARM_ERROR_COMPLETION_QUEUE,
  RDARM_ERROR_COMPLETION_CHANNEL,
  RDARM_ERROR_NOTIFY_CQ,
  RDARM_ERROR_REGISTER_MR,
  RDARM_ERROR_POST_WR,
  RDARM_ERROR_RESOLVE_ADDRESS,
  RDARM_ERROR_CONFLICT_CONNECTION,
  RDARM_ERROR_ACCEPT_CONNECTION,
  RDARM_ERROR_CQ_EVENT,
  RDARM_ERROR_CREATE_CONNECTION,
  RDARM_ERROR_CONNECT,
  RDARM_ERROR_UNKNOWN_RDMA_OP_CODE,
  RDARM_ERROR_UNKNOWN_REQUEST_TYPE,
  RDARM_ERROR_UNKNOWN_OPERATION_TYPE,
  RDARM_ERROR_NO_FREE_COMMUNICATE_BUFFER,
  RDARM_ERROR_JOIN_ERROR,

  RDARM_ERROR_TOO_LONG_BATCH_OPERATION,
  RDARM_ERROR_REMOTE_OPERATION,

  RDARM_ERROR_UNKNOWN = -100,
};

/**
 * @brief Get a readable message.
 */
const char *rdarm_error_string(enum rdarm_error_code error);

/**
 * @brief The result type of operation.
 */
enum rdarm_operation_result_type {
  RDARM_OP_SUCCESS = 0,
  RDARM_OP_UNDEFINED,
  RDARM_OP_FAILED_TO_SEND_REMOTE,
  RDARM_OP_NOT_READY,
  RDARM_OP_INVALID_KEY_OR_VALUE,
  RDARM_OP_INVALID_KEY_LENGTH,
  RDARM_OP_INVALID_TABLE_ID,
  RDARM_OP_KEY_NOT_EXIST,
  RDARM_OP_CONFLICT_VALUE_TYPE,
  RDARM_OP_EMPTY_LIST,
  RDARM_OP_TABLE_IS_FULL,
  RDARM_OP_NO_CACHE,
  RDARM_OP_BATCH_PROCESSING,
};

/**
 * @brief Get a readable message of operation result.
 */
const char *rdarm_operation_string(enum rdarm_operation_result_type op);

#endif //RDARM_INCLUDE_RDARM_COMMON_H_
