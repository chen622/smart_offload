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

#define TOTAL_FLOW 30000000

bool is_running = true;

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    dzlog_info("Signal %d received, preparing to exit...",
               signum);
    is_running = false;
  }
}

int main(int argc, char **argv) {
  int ret = 0;

  ret = zlog_init("conf/zlog.conf");
  if (ret) {
    printf("zlog init failed\n");
    return -1;
  };

  zlog_category_t *logger = zlog_get_category("main");
  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    zlog_error(logger, "invalid EAL arguments\n");
    ret = -2;
    goto rte_err;
  }
  struct smto *smto_cb;
  ret = init_smto(&smto_cb);
  if (ret != SMTO_SUCCESS) {
    zlog_error(logger, "init smto failed: %s\n", smto_error_string(ret));
    ret = -3;
    goto smto_err;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  zlog_info(logger, "SmartOffload Flow Benchmark start running!");

  is_running = true;

  struct timeval start, end;
  gettimeofday(&start, NULL);
  long flow_count = 0;
  struct rte_flow_error flow_error = {0};

  struct smto_flow_key *flow_keys = rte_calloc("flow_keys", TOTAL_FLOW, sizeof(struct smto_flow_key), 0);
  struct smto_flow_key flow_key = {
      .tuple = {
          .proto = IPPROTO_TCP,
          .ip1 = RTE_IPV4(1, 1, 1, 1),
          .port1 = 10,
          .port2 = 1,
          .ip2 = RTE_IPV4(2, 2, 2, 2)
      }
  };
  while (flow_count < TOTAL_FLOW) {
    rte_memcpy(&flow_keys[flow_count], &flow_key, sizeof(struct smto_flow_key));
    struct rte_flow *flow = create_general_offload_flow(0, &flow_keys[flow_count], &flow_error);
    if (flow == NULL) {
      zlog_error(smto_cb->logger, "cannot create a offload flow of packet: %s", flow_error.message);
      goto flow_err;
    }
    flow_key.tuple.port2 = (flow_key.tuple.port2 + 1) % 50000;
    flow_key.tuple.ip2++;
    flow_keys[flow_count].flow = flow;
    flow_count++;
  }
  gettimeofday(&end, NULL);
  long time_use = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
  zlog_info(smto_cb->logger, "total has %ld flows and take %ld us, average %f us", flow_count, time_use,
         (double) time_use / (double) flow_count);

  while (is_running);

  zlog_info(logger, "SmartOffload stop running!");
  flow_err:
  rte_flow_flush(0, &flow_error);
  rte_free(flow_keys);
  destroy_smto(smto_cb);
  smto_err:
  rte_eal_cleanup();
  rte_err:
  zlog_fini();
  exit(ret);
}