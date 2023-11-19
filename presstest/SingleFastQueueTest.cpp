#include "../src/utils/FastQueue.h"
#include "../src/utils/SingleQueue.h"

#include "PressTest.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace storage {
void TestSingleQueue(uint64_t count) {
  SingleQueue<int64_t, 100> squeue;
  vector<int64_t *> vct_ptr;
  vct_ptr.reserve(1000000);
  for (size_t i = 0; i < 1000000; i++) {
    vct_ptr.push_back(new int64_t);
    vct_ptr[i] = 0;
  }

  thread *tAr = new thread[2];
  chrono::system_clock::time_point st = chrono::system_clock::now();
  tAr[0] = thread([vct_ptr, squeue]() {
    for (uint64_t j = 0; j < count; j++) {
      int64_t *ptr = vct_ptr[j % 1000000];
      *ptr = j;
      squeue.Push(ptr, j % 30 == 0);
    }

    squeue.Submit();
  });

  tAr[1] = thread([vct_ptr, squeue]() {
    queue<T *> queue;
    int j = 0;
    while (j < count) {
      squeue.Pop(queue);
      while (queue.size() > 0) {
        int64_t *ptr = queue.front();
        queue.pop();
        if (*ptr != j) {
          cout << j << ": " << *ptr;
        }
        j++;
      }
    }
  });

  tAr[1].join();
}

void TestFastQueue(uint16_t threads, uint64_t count) {}
} // namespace storage