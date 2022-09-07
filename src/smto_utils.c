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

#include "internal/smto_utils.h"

pthread_barrier_t the_barrier;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

uint64_t *nums; /**< The array of time */
int length; /**< The length of array */
int block_length; /**< The length of each sub array */

static void adjust(int i, int n, long turn) {
  int parent = i;
  int child;
  uint64_t temp = nums[i + turn * block_length];

  while (parent * 2 + 1 <= n - 1) {
    child = parent * 2 + 1;
    if (child != n - 1 && nums[child + turn * block_length] < nums[child + turn * block_length + 1])
      child++;
    if (temp >= nums[child + turn * block_length])
      break;
    nums[parent + turn * block_length] = nums[child + turn * block_length];
    parent = child;
  }
  nums[parent + turn * block_length] = temp;
}

static void *heap_sort(void *args) {
  long turn = (long) args;

  for (int i = (block_length - 2) / 2; i >= 0; i--)
    adjust(i, block_length, turn);

  pthread_mutex_lock(&lock);
  pthread_mutex_unlock(&lock);

  for (int j = block_length - 1; j > 0; j--) {
    uint64_t temp = nums[0 + turn * block_length];
    nums[0 + turn * block_length] = nums[j + turn * block_length];
    nums[j + turn * block_length] = temp;
    adjust(0, j, turn);
  }

  pthread_mutex_lock(&lock);
  pthread_mutex_unlock(&lock);

  pthread_barrier_wait(&the_barrier);
  return NULL;
}

static void merge(int start1, int start2, int end) {
  uint64_t *a = calloc(end - start1 + 1, sizeof(long));
  int k = 0;
  int i = start1;
  int j = start2;

  while (i <= start2 - 1 && j <= end) {
    if (nums[i] <= nums[j]) {
      a[k] = nums[i];
      k++;
      i++;
    } else {
      a[k] = nums[j];
      k++;
      j++;
    }
  }

  while (i <= start2 - 1) {
    a[k] = nums[i];
    k++;
    i++;
  }
  while (j <= end) {
    a[k] = nums[j];
    k++;
    j++;
  }

  for (i = start1, k = 0; i <= end; i++, k++)
    nums[i] = a[k];

}

void time_stat(zlog_category_t *logger, uint64_t time_array[], int size, char *prefix_name) {
  uint64_t sum = 0;
  for (int j = 0; j < size; ++j) {
    sum += time_array[j];
  }

  pthread_barrier_init(&the_barrier, NULL, TURN + 1);

  nums = time_array;
  length = size;
  block_length = size / TURN;
  pthread_t tid[TURN];
  for (long i = 0; i < TURN; i++)
    pthread_create(&tid[i], NULL, heap_sort, (void *) i);

  pthread_barrier_wait(&the_barrier);

  int gap = length / TURN;
  for (; gap < length && gap != 0; gap *= 2) {
    int i;
    for (i = 0; i + gap * 2 - 1 < length; i += gap * 2)
      merge(i, i + gap, i + gap * 2 - 1);
    if (i + gap < length)
      merge(i, i + gap, length - 1);
  }

  zlog_info(logger, "%s: MIN: %ld, 50%%: %ld, 90%%: %ld, 99%%: %ld, MAX: %ld, AVG: %ld",
            prefix_name,
            time_array[0],
            time_array[size / 2],
            time_array[size * 4 / 5],
            time_array[size * 99 / 100],
            time_array[size - 1],
            sum / size);
}