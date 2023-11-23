#include "../src/utils/FastQueue.h"
#include "../src/utils/STQueue.h"
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
  SingleQueue<int64_t, 1000> squeue;
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
      squeue.Push(ptr, j % 100 == 0);
    }

    squeue.Submit();
    cout << "produce end." << endl;
  });

  tAr[1] = thread([&vct_ptr, &squeue, count]() {
    MDeque<int64_t *> queue;
    int j = 1;
    int num = 0;
    while (j <= count) {
      num++;
      squeue.Pop(queue);
      if (queue.size() == 0) {
        std::this_thread::yield();
        continue;
      }

      while (queue.size() > 0) {
        int64_t *ptr = queue.front();
        queue.pop_front();
        if (*ptr != j) {
          cout << j << ": " << *ptr << endl;
          abort();
        }
        j++;
      }
    }

    cout << "consume end. num: " << num << endl;
  });

  tAr[1].join();
  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  cout << "Time(ms):" << duration.count() << "\tCount:" << squeue._testCount
       << endl;
}

void TestSTQueue(uint64_t count) {
  STQueue<int64_t, 1000> squeue;
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
      while (!squeue.Push(ptr, j % 200 == 0)) {
        std::this_thread::yield();
      }
    }

    squeue.Submit();
    cout << "produce end." << endl;
  });

  tAr[1] = thread([&vct_ptr, &squeue, count]() {
    MVector<int64_t *> vct;
    int j = 1;
    int num = 0;
    while (j <= count) {
      num++;
      squeue.Pop(vct);
      if (vct.size() == 0) {
        std::this_thread::yield();
        continue;
      }

      for (int64_t *ptr : vct) {
        if (*ptr != j) {
          cout << j << ": " << *ptr << endl;
          abort();
        }
        j++;
      }

      vct.clear();
    }

    cout << "consume end. num: " << num << endl;
  });

  tAr[1].join();
  chrono::system_clock::time_point et = chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(et - st);
  cout << "Time(ms):" << duration.count() << "\tCount:" << count << endl;
}

void TestFastQueue(uint16_t threads, uint64_t count) {
  FastQueue<int64_t, 1000> fqueue(threads);
  vector<vector<int64_t *>> v_vct_ptr(threads);
  for (uint16_t k = 0; k < threads; k++) {
    v_vct_ptr[k].reserve(10000000);
    for (size_t i = 0; i < 10000000; i++) {
      v_vct_ptr[k].push_back(new int64_t(0));
    }
  }

  thread *tAr = new thread[threads];
  chrono::system_clock::time_point st = chrono::system_clock::now();
  for (uint16_t i = 0; i < threads; i++) {
    tAr[i] = thread([i, &v_vct_ptr, &fqueue, count]() {
      auto &vct_ptr = v_vct_ptr[i];
      int64_t pre_v = ((int64_t)i << 48);
      for (uint64_t j = 1; j <= count; j++) {
        int64_t *ptr = vct_ptr[j % 10000000];
        *ptr = pre_v + j;
        fqueue.Push(i, ptr, j % 100 == 0);
      }

      fqueue.Submit(i);
      cout << "produce end, i=" << i << endl;
    });
  }

  thread cm([&fqueue, count, threads]() {
    vector<int64_t> v_r(count);
    for (uint64_t k = 0; k < count; k++) {
      v_r[k] = (k << 48) + 1;
    }

    uint64_t t_count = 0;
    uint64_t total = count * threads;
    MDeque<int64_t *> queue;
    int num = 0;

    while (t_count < total) {
      num++;
      fqueue.Pop(queue);
      if (queue.size() == 0) {
        std::this_thread::yield();
        continue;
      }

      while (queue.size() > 0) {
        int64_t *ptr = queue.front();
        queue.pop_front();
        uint64_t idx = (*ptr) >> 48;
        if (*ptr != v_r[idx]++) {
          cout << "idx: " << idx << "  t_count:" << t_count << "  val: " << *ptr
               << endl;
          abort();
        }
        t_count++;
      }
    }

    cout << "consume end. num: " << num << endl;
  });
}
} // namespace storage