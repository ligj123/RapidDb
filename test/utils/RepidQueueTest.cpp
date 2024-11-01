#include "../../src/dataType/DataValueDigit.h"
#include "../../src/utils/Log.h"
#include "../../src/utils/RapidQueue.h"
#include "../../src/utils/ThreadPool.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include <thread>

using namespace std;

namespace storage {
BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(LineQueue_test) {
  class LineqQueueEx : public LineQueue<uint64_t> {
  public:
    using LineQueue<uint64_t>::_startNode;
    using LineQueue<uint64_t>::_endNode;
    using LineQueue<uint64_t>::_head;
    using LineQueue<uint64_t>::_submited;
    using LineQueue<uint64_t>::_tail;
  };

  const uint64_t CNT = 1000000;
  uint64_t *arr = new uint64_t[CNT];
  for (size_t i = 0; i < CNT; i++) {
    arr[i] = i;
  }
  LineqQueueEx lq;
  thread t([&lq, &CNT]() {
    size_t val = 0;
    MDeque<uint64_t *> mq;

    while (val < CNT) {
      lq.Pop(mq);

      while (mq.size() > 0) {
        uint64_t *p = mq.front();
        mq.pop_front();
        if (val != *p || val % (CNT / 10) == 0) {
          BOOST_TEST(val == *p);
        }

        val++;
      }
    }
  });

  for (size_t i = 0; i < CNT; i++) {
    lq.Push(&arr[i], false);
  }
  lq.Submit();
  t.join();

  BOOST_TEST(lq._startNode == lq._endNode);
  BOOST_TEST(lq._head == lq._submited.load(memory_order_relaxed));
  BOOST_TEST(lq._head == lq._tail.load(memory_order_relaxed));
  BOOST_TEST(lq.IsEmpty());
  BOOST_TEST(lq.RoughSize() == 0);

  lq._head = ELE_SIZE_NOT;
  lq._submited.store(ELE_SIZE_NOT, memory_order_relaxed);
  lq._tail.store(ELE_SIZE_NOT, memory_order_relaxed);

  for (size_t i = 0; i < CNT; i++) {
    if (i % 10000 == 0) {
      BOOST_TEST(lq.RoughSize() == i);
    }

    lq.Push(&arr[i]);
  }
  lq.Submit();

  BOOST_TEST(lq._tail.load(memory_order_relaxed) == ELE_SIZE_NOT);
  BOOST_TEST(lq._head == lq._submited.load(memory_order_relaxed));
  BOOST_TEST(lq._head == (CNT - 32));
  delete[] arr;
}

BOOST_AUTO_TEST_CASE(RapidQueue_test) {
  const uint64_t CNT = 100000;
  uint64_t *arr = new uint64_t[CNT];
  for (size_t i = 0; i < CNT; i++) {
    arr[i] = 0;
  }

  RapidQueue<uint64_t> rq(10, 10);
  thread tAr1[10];
  for (size_t i = 0; i < 10; i++) {
    tAr1[i] = thread([&rq, &CNT, arr, i]() {
      size_t idx = i;
      LOG_INFO << "IDX1: " << idx;
      for (size_t j = 0; j < CNT; j++) {
        rq.Push(idx, &arr[j], false);
      }

      rq.Submit(idx);
    });
  }

  size_t count = 0;
  MDeque<uint64_t *> mq;
  while (count < CNT * 10) {
    rq.Pop(mq);

    for (auto iter = mq.begin(); iter != mq.end(); iter++) {
      *(*iter) += 1;
    }

    count += mq.size();
    mq.clear();
  }

  LOG_INFO << "STEP1";
  for (size_t i = 0; i < CNT; i++) {
    if (arr[i] != 10) {
      BOOST_TEST(arr[i] == 10);
    }
  }

  mq.clear();
  bool bstop = false;
  thread tpop([&rq, &mq, &bstop]() {
    while (!bstop) {
      rq.Pop(mq);
    }
  });

  thread tAr2[10];
  for (size_t i = 0; i < 10; i++) {
    tAr2[i] = thread([&rq, &CNT, arr, i]() {
      size_t idx = i;
      LOG_INFO << "IDX2: " << idx;
      for (size_t j = 0; j < CNT; j++) {
        rq.Push(idx, &arr[j], false);
      }

      rq.Submit(idx);
    });
  }

  for (size_t i = 0; i < 10; i++) {
    tAr2[i].join();
  }

  rq.ResetLiveThreadNumber(5);
  thread tAr3[10];
  for (size_t i = 0; i < 5; i++) {
    tAr3[i] = thread([&rq, &CNT, arr, i]() {
      size_t idx = i;
      LOG_INFO << "IDX3: " << idx;
      for (size_t j = 0; j < CNT; j++) {
        rq.Push(idx, &arr[j], false);
      }

      rq.Submit(idx);
    });
  }

  for (size_t i = 0; i < 10; i++) {
    tAr3[i].join();
  }

  bstop = true;
  atomic_thread_fence(std::memory_order_release);
  tpop.join();

  BOOST_TEST(mq.size() == CNT * 15);
  LOG_INFO << "END";
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
