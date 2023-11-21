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
  vct_ptr.reserve(10000000);
  for (size_t i = 0; i < 10000000; i++) {
    vct_ptr.push_back(new int64_t);
    *vct_ptr[i] = 0;
  }

  thread *tAr = new thread[2];
  chrono::system_clock::time_point st = chrono::system_clock::now();
  tAr[0] = thread([&vct_ptr, &squeue, count]() {
    for (uint64_t j = 1; j <= count; j++) {
      int64_t *ptr = vct_ptr[j % 10000000];
      *ptr = j;
      while (!squeue.Push(ptr, j % 30 == 0)) {
        std::this_thread::yield();
      }
    }

    squeue.Submit();
    cout << "produce end." << endl;
  });

  tAr[1] = thread([&vct_ptr, &squeue, count]() {
    queue<int64_t *> queue;
    int j = 1;
    while (j <= count) {
      squeue.Pop(queue);
      if (queue.size() == 0) {
        std::this_thread::yield();
        continue;
      }

      while (queue.size() > 0) {
        int64_t *ptr = queue.front();
        queue.pop();
        if (*ptr != j) {
          cout << j << ": " << *ptr << endl;
          abort();
        }
        j++;
      }
    }

    cout << "consume end." << endl;
  });

  tAr[1].join();
  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  cout << "Time(ms):" << duration.count() << "\tCount:" << count << endl;
}

void TestFastQueue(uint16_t threads, uint64_t count) {}
} // namespace storage