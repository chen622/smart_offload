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
#include <sys/time.h>
#include <signal.h>

#include "smto.h"
#include "internal/smto_flow_key.h"
#include "internal/smto_flow_engine.h"

#define START_FLOW 32
#define END_FLOW 10000000
#define ADD_VALUE 100000

#define BURST_SIZE 32

bool is_running = true;

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    dzlog_info("Signal %d received, preparing to exit...",
               signum);
    is_running = false;
  }
}

static long flow_count = 0;
static struct smto *smto_test_cb = 0;
static struct smto_flow_key *flow_keys = 0;
static struct smto_flow_key flow_key = {
    .tuple = {
        .proto = IPPROTO_TCP,
        .ip1 = RTE_IPV4(1, 1, 1, 1),
        .port1 = 10,
        .port2 = 1,
        .ip2 = RTE_IPV4(2, 2, 2, 2)
    }
};



//static pthread_mutex_t mutex;
///**
// * Create offload flow rule with sub-thread.
// * @param flow_amount The amount to create.
// */
//void *pthread_create_flow(void *flow_amount) {
//  int flow_num = (int) flow_amount;
//  uint32_t i;
//  struct rte_flow *flow;
//  struct rte_flow_error flow_error = {0};
//
//  for (i = 0; i < flow_num; i++) {
//    pthread_mutex_lock(&mutex);
//    flow_key.ip_dst++;
//    flow = create_general_offload_flow(0, &flow_keys[flow_count], &flow_error);
//    if (flow == NULL) {
//      zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
//      return NULL;
//    }
//    flow_keys[flow_count].flow = flow;
//    flow_count++;
//    pthread_mutex_unlock(&mutex);
//  }
//  return NULL;
//}

//static struct rte_ring *flow_ring = 0;
///**
// * Generate flow rules into the ring.
// * @param args
// * @return
// */
//void *pthread_generate_flow(void *args) {
//  uint32_t i;
//
//  for (i = 0; i < END_FLOW; i++) {
//    flow_keys[i] = flow_key;
//    flow_keys[i].ip_dst += i;
//    rte_ring_enqueue(flow_ring, &flow_keys[i]);
//    rte_delay_us_sleep(10);
//  }
//  return NULL;
//}

int main(int argc, char **argv) {
  int ret = 0;

  const int thread_num = 10;
  pthread_t threads[thread_num];

  ret = zlog_init("conf/zlog.conf");
  if (ret) {
    printf("zlog init failed\n");
    return -1;
  };

  zlog_category_t *benchmark_logger = zlog_get_category("benchmark");
  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    zlog_error(benchmark_logger, "invalid EAL arguments\n");
    ret = -2;
    goto rte_err;
  }

  ret = init_smto(&smto_test_cb);
  if (ret != SMTO_SUCCESS) {
    zlog_error(smto_test_cb->logger, "init smto failed: %s\n", smto_error_string(ret));
    ret = -3;
    goto smto_err;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  zlog_info(smto_test_cb->logger, "SmartOffload Flow Benchmark start running!");

  is_running = true;

  flow_keys = rte_calloc("flow_keys", END_FLOW, sizeof(struct smto_flow_key), 0);
  for (int i = 0; i < END_FLOW; i++) {
    flow_keys[i] = flow_key;
    flow_key.tuple.port2 = (flow_key.tuple.port2 + 1) % 50000;
    flow_key.tuple.ip2++;
  }

//  flow_ring = rte_ring_create("flow_ring", 1024 * 64, rte_socket_id(), RING_F_MP_RTS_ENQ | RING_F_SC_DEQ);

  struct rte_flow_error flow_error = {0};

  //< Warm up the flow engine.
  for (int i = 0; i < 1000; i++) {
    struct rte_flow *flow = create_general_offload_flow(0, &flow_keys[i], &flow_error);
    if (flow == NULL) {
      zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
      goto flow_err;
    }
    flow_keys[i].flow = flow;
    rte_flow_flush(0, &flow_error);
  }
  zlog_info(smto_test_cb->logger, "Warm up finished, start benchmarking!");

  //< Create flow with different amount.
//  for (int i = START_FLOW; i < END_FLOW; i += ADD_VALUE) {
//    struct timeval start, end;
//    gettimeofday(&start, NULL);
//    long flow_count = 0;
//    while (flow_count < i) {
//      struct rte_flow *flow = create_general_offload_flow(0, &flow_keys[flow_count], &flow_error);
//      if (flow == NULL) {
//        zlog_error(smto_test_cb->smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
//        goto flow_err;
//      }
//      flow_keys[flow_count].flow = flow;
//      flow_count++;
//    }
//    gettimeofday(&end, NULL);
//    long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
//    rte_flow_flush(0, &flow_error);
//    zlog_info(smto_test_cb->logger, "total has %ld flows and take %ld us, average %f us", flow_count, time_use,
//              (double) time_use / (double) flow_count);
//    rte_delay_us_sleep(1 * 1000 * 1000);
//  }

  //< Create flow with different amount (use clock_gettime).
  long sum_time;
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 1; i < END_FLOW + 1; ++i) {
    flow_keys[i] = flow_key;
    flow_keys[i].tuple.ip2++;
    struct rte_flow *flow = create_general_offload_flow(0, &flow_keys[i], &flow_error);
    if (flow == NULL) {
      zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
      goto flow_err;
    }
    flow_keys[i].flow = flow;
    if (i % 10000 == 0) {
      clock_gettime(CLOCK_MONOTONIC, &end);
      long time_use = 1000 * 1000 * 1000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
      sum_time += time_use;
      zlog_info(benchmark_logger, "%lf %lf", (double)sum_time / 1000.0 / 1000.0 / 1000.0, 10000.0 / ((double)time_use / 1000.0 / 1000.0/ 1000.0));
      clock_gettime(CLOCK_MONOTONIC, &start);
    }
  }

  //< Create flow with multi-thread.
//  long sum_time;
//  struct timeval start, end;
//  pthread_mutex_init(&mutex, NULL);
//  pthread_mutex_lock(&mutex);
//  for (int i = 0; i < 10; ++i) {
//    ret = pthread_create(&threads[i], NULL, pthread_create_flow, (void *) (intptr_t) (END_FLOW / thread_num));
//    if (ret != 0) {
//      zlog_error(smto_test_cb->logger, "cannot create a thread: %s", strerror(ret));
//      goto flow_err;
//    }
//  }
//  pthread_mutex_unlock(&mutex);
//  gettimeofday(&start, NULL);
//  while (true) {
//    if (flow_count >= END_FLOW) {
//      gettimeofday(&end, NULL);
//      long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
//      zlog_info(benchmark_logger, "%ld %ld", time_use, flow_count);
//      break;
//    } else {
//      gettimeofday(&end, NULL);
//      long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
//      zlog_info(benchmark_logger, "%ld %ld", time_use, flow_count);
//      rte_delay_us_sleep(400 * 1000);
//    }
//  }
//  for (int i = 0; i < thread_num; ++i) {
//    pthread_join(threads[i], NULL);
//  }

    //< Generate flow with multi-thread and apply flow with single thread.
//  struct timeval start, end;
//  pthread_create(&threads[0], NULL, pthread_generate_flow, NULL);
//  gettimeofday(&start, NULL);
//  while (flow_count < END_FLOW) {
//    struct smto_flow_key *new_key = 0;
//    while (rte_ring_dequeue(flow_ring, (void **) &new_key) != 0) {
//      rte_delay_us_sleep(100);
//    }
//    struct rte_flow *flow = create_general_offload_flow(0, new_key, &flow_error);
//    if (flow == NULL) {
//      zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
//      goto flow_err;
//    }
//    new_key->flow = flow;
//    flow_count++;
//  }
//  gettimeofday(&end, NULL);
//  rte_flow_flush(0, &flow_error);
//  long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
//  zlog_info(smto_test_cb->logger, "continuously %ld", time_use);
//  pthread_join(threads[0], NULL);

    //< Generate flow with multi-thread and apply flow with multi-thread and burst.
//  pthread_create(&threads[0], NULL, pthread_generate_flow, NULL);
//  gettimeofday(&start, NULL);
//  flow_count = 0;
//  while (flow_count < END_FLOW) {
//    struct smto_flow_key *new_key[BURST_SIZE] = {0};
//    uint32_t last = 0;
//    uint32_t count = rte_ring_dequeue_burst(flow_ring, (void **) &new_key, BURST_SIZE, &last);
//    for (int i = 0; i < count; ++i) {
//      struct rte_flow *flow = create_general_offload_flow(0, new_key[flow_count], &flow_error);
//      if (flow == NULL) {
//        zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
//        goto flow_err;
//      }
//      new_key[flow_count]->flow = flow;
//      flow_count++;
//    }
//    if (last == 0) {
//        rte_delay_us_sleep(1000);
//    }
//  }
//  gettimeofday(&end, NULL);
//  rte_flow_flush(0, &flow_error);
//  time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
//  zlog_info(smto_test_cb->logger, "discontinuity %ld", time_use);
//  pthread_join(threads[0], NULL);


    //< Apply flows with different duration.
//  struct timeval start, end;
//  for (int i = 100; i < 1000; i += 100) {
//    struct rte_flow *flow = create_general_offload_flow(0, &flow_keys[flow_count], &flow_error);
//    if (flow == NULL) {
//      zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
//      goto flow_err;
//    }
//    flow_keys[flow_count++].flow = flow;
////    rte_flow_flush(0, &flow_error);
////    rte_delay_us_block(i * 1000);
//    rte_delay_us_sleep(i * 10000);
//
//    gettimeofday(&start, NULL);
//    flow = create_general_offload_flow(0, &flow_keys[flow_count], &flow_error);
//    if (flow == NULL) {
//      zlog_error(smto_test_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
//      goto flow_err;
//    }
//    flow_keys[flow_count++].flow = flow;
//    gettimeofday(&end, NULL);
//
//    long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
//    zlog_info(smto_test_cb->logger, "duration:%d delay:%ld", i, time_use);
//    rte_flow_flush(0, &flow_error);
//    rte_delay_us_block(1000 * 1000);
//  }

//  while (is_running);

  zlog_info(smto_test_cb->logger, "SmartOffload stop running!");
  flow_err:
  rte_flow_flush(0, &flow_error);
  rte_free(flow_keys);
  destroy_smto(smto_test_cb);
  smto_err:
  rte_eal_cleanup();
  rte_err:
  zlog_fini();
  exit(ret);
}