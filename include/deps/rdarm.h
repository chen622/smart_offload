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

#ifndef RDARM_INCLUDE_RDARM_H_
#define RDARM_INCLUDE_RDARM_H_

#include "rdarm_common.h"
#include "rdarm_internal/rdarm_store.h"

/**
 * @brief The main structure of rdarm
 */
typedef struct rdarm {
  rdarm_node self;
  key_slot *key_slots;
  struct ibv_mr *key_slots_mr;
  pthread_rwlock_t table_lock[RDARM_SLOT_NUM][RDARM_HASH_TABLE_NUM];
  zlog_category_t *logger;
  rdarm_node *others;
} rdarm;

struct general_args {
  rdarm *rdarm_cb;
  rdarm_node *node;
  rdarm_connection *connection;
};


/**
 * @brief Initialize rdarm.
 *
 * @param rdarm_cb The pointer to the new rdarm. Need to be free by rdarm_destroy.
 * @param self_ip The ip address of this node. 0.0.0.0 means standalone mode.
 * @param peer_ip The ip address of peer node. Optional, 0.0.0.0 means without peer node.
 *
 * @return The result of initialization. Get the error msg by calling rdarm_error_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_init(rdarm **rdarm_cb,
               const char self_ip[INET_ADDRSTRLEN],
               const char peer_ip[INET_ADDRSTRLEN]);

/**
 * @biref Destroy the rdarm.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 *
 * @return The result of destruction. Get the error msg by calling rdarm_error_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_destroy(rdarm *rdarm_cb);

/**
 * @brief Set a string value to a string key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key which will be set.
 * @param value The value which will be set.
 * @param table_id The hash table user selected.
 *
 * @return The result of setting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_set_string_string(rdarm *rdarm_cb, char *key, char *value, uint16_t table_id);

/**
 * @brief Get a string value of a string key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key which will be get.
 * @param value The value which will be save to.
 * @param table_id The hash table user selected.
 *
 * @return The result of getting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_get_string_string(rdarm *rdarm_cb, char *key, char **value, uint16_t table_id);

/**
 * @brief Push a uint64 value into a list with a five-tuple key from left side.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key which will be set.
 * @param value The value which will be set.
 * @param table_id The hash table user selected.
 *
 * @return The result of pushing. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_lpush_tuple_uint64(rdarm *rdarm_cb, rdarm_five_tuple *key, uint64_t value, uint16_t table_id);

/**
 * @brief Pop a uint64 value from a list with a five-tuple key from left side.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key which will be get.
 * @param value The value which will be save to.
 * @param table_id The hash table user selected.
 *
 * @return The result of pushing. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_lpop_tuple_uint64(rdarm *rdarm_cb, rdarm_five_tuple *key, uint64_t *value, uint16_t table_id);

/**
 * @brief Set a uint64 value with a five-tuple key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key of this value.
 * @param value The value to set.
 * @param table_id The hash table user selected.
 *
 * @return The result of setting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_set_tuple_uint64(rdarm *rdarm_cb, rdarm_five_tuple *key, uint64_t value, uint16_t table_id);

/**
 * @brief Get a uint64 value with a five-tuple key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key of this value.
 * @param value The value which will be save.
 * @param table_id The hash table user selected.
 *
 * @return The result of getting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_get_tuple_uint64(rdarm *rdarm_cb, rdarm_five_tuple *key, uint64_t *value, uint16_t table_id);

/**
 * @brief Get a uint64 value with a five-tuple key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key of this value.
 * @param value The value which will be save.
 * @param table_id The hash table user selected.
 *
 * @return The result of getting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_get_tuple_uint64_with_cache(rdarm *rdarm_cb, rdarm_five_tuple *key, uint64_t *value, uint16_t table_id);

int rdarm_get_tuple_uint64_with_batch(rdarm *rdarm_cb,
                                      rdarm_five_tuple keys[],
                                      uint64_t values[],
                                      uint16_t table_ids[],
                                      uint16_t batch_size,
                                      int results[]);

/**
 * @brief Add a uint64 value with a five-tuple key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key of this value.
 * @param value The value which will be add.
 * @param table_id The hash table user selected.
 *
 * @return The result of getting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_add_tuple_uint64(rdarm *rdarm_cb, rdarm_five_tuple *key, uint64_t value, uint16_t table_id);

/**
 * @brief Set a uint64 value with a string key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key of this value.
 * @param value The value to set.
 * @param table_id The hash table user selected.
 *
 * @return The result of setting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_set_string_uint64(rdarm *rdarm_cb, char *key, uint64_t value, uint16_t table_id);

/**
 * @brief Get a uint64 value with a string key.
 *
 * @param rdarm_cb The pointer to a exist rdarm.
 * @param key The key of this value.
 * @param value The value which will be save.
 * @param table_id The hash table user selected.
 *
 * @return The result of getting. Get the error msg by calling rdarm_operation_string().
 * @retval 0     Success.
 * @retval Not 0 Failed.
 */
int rdarm_get_string_uint64(rdarm *rdarm_cb, char *key, uint64_t *value, uint16_t table_id);

#endif //RDARM_INCLUDE_RDARM_H_
