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
          LOG_INFO << val;
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

  LOG_INFO << "second";
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
  LOG_INFO << "END";
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
  }
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace storage
