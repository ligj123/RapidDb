#include <boost/test/unit_test.hpp>
#include "../../src/utils/SpinMutex.h"

namespace utils {
BOOST_AUTO_TEST_SUITE(SpinMutexTest)

BOOST_AUTO_TEST_CASE(SpinMutex_test)
{
  SpinMutex sm;
  BOOST_TEST(sm.try_lock());
  BOOST_TEST(sm.is_locked());
  sm.unlock();
  sm.lock();
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(!sm.try_lock());
  sm.unlock();
  BOOST_TEST(!sm.is_locked());

  sm.lock();
  uint64_t ms = 0;
  std::thread t([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock();
  t.join();
  BOOST_TEST(ms > 10);
}

BOOST_AUTO_TEST_CASE(SharedSpinMutex_test)
{
  SharedSpinMutex sm;
  BOOST_TEST(sm.try_lock());
  BOOST_TEST(!sm.try_lock_shared());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 0);
  sm.unlock();

  BOOST_TEST(sm.try_lock_shared());
  BOOST_TEST(!sm.try_lock());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(!sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 1);
  sm.lock_shared();
  BOOST_TEST(sm.read_locked_count() == 2);
  sm.unlock_shared();
  sm.unlock_shared();

  sm.lock();
  BOOST_TEST(!sm.try_lock());
  BOOST_TEST(!sm.try_lock_shared());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 0);
  sm.unlock();

  sm.lock_shared();
  BOOST_TEST(sm.try_lock_shared());
  BOOST_TEST(!sm.try_lock());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(!sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 2);
  sm.unlock_shared();
  sm.unlock_shared();

  sm.lock();
  uint64_t ms = 0;
  std::thread t1([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock();
  t1.join();
  BOOST_TEST(ms > 10);

  sm.lock();
  ms = 0;
  std::thread t2([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock_shared();
    sm.unlock_shared();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock();
  t2.join();
  BOOST_TEST(ms > 10);

  sm.lock_shared();
  ms = 0;
  std::thread t3([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock_shared();
  t3.join();
  BOOST_TEST(ms > 10);
}

BOOST_AUTO_TEST_CASE(ReentrantSpinMutex_test)
{
  ReentrantSpinMutex sm;
  BOOST_TEST(sm.try_lock());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(sm.reentrant_count() == 1);
  sm.lock();
  BOOST_TEST(sm.reentrant_count() == 2);
  sm.unlock();
  sm.unlock();

  sm.lock();
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(sm.try_lock());
  BOOST_TEST(sm.reentrant_count() == 2);
  sm.unlock();
  BOOST_TEST(sm.is_locked());
  sm.unlock();
  BOOST_TEST(!sm.is_locked());

  sm.lock();
  uint64_t ms = 0;
  std::thread t1([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(12));
  sm.unlock();
  t1.join();
  BOOST_TEST(ms > 10);

  sm.lock();
  ms = 0;
  std::thread t2([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  sm.lock();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  sm.unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  sm.unlock();
  t2.join();
  BOOST_TEST(ms > 10);
}

BOOST_AUTO_TEST_CASE(ReentrantSharedSpinMutex_test)
{
  ReentrantSharedSpinMutex sm;
  BOOST_TEST(sm.try_lock());
  BOOST_TEST(!sm.try_lock_shared());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(sm.is_write_locked());
  BOOST_TEST(sm.reentrant_count() == 1);
  BOOST_TEST(sm.read_locked_count() == 0);
  sm.unlock();

  BOOST_TEST(sm.try_lock_shared());
  BOOST_TEST(!sm.try_lock());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(!sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 1);
  sm.lock_shared();
  BOOST_TEST(sm.read_locked_count() == 2);
  BOOST_TEST(sm.reentrant_count() == 0);
  sm.unlock_shared();
  sm.unlock_shared();

  sm.lock();
  BOOST_TEST(sm.try_lock());
  BOOST_TEST(sm.reentrant_count() == 2);
  BOOST_TEST(!sm.try_lock_shared());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 0);
  sm.unlock();
  sm.unlock();

  sm.lock_shared();
  BOOST_TEST(sm.try_lock_shared());
  BOOST_TEST(!sm.try_lock());
  BOOST_TEST(sm.is_locked());
  BOOST_TEST(!sm.is_write_locked());
  BOOST_TEST(sm.read_locked_count() == 2);
  BOOST_TEST(sm.reentrant_count() == 0);
  sm.unlock_shared();
  sm.unlock_shared();

  sm.lock();
  uint64_t ms = 0;
  std::thread t1([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock();
  t1.join();
  BOOST_TEST(ms > 10);

  sm.lock();
  ms = 0;
  std::thread t2([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock_shared();
    sm.unlock_shared();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock();
  t2.join();
  BOOST_TEST(ms > 10);

  sm.lock_shared();
  ms = 0;
  std::thread t3([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock();
    sm.unlock();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock_shared();
  t3.join();
  BOOST_TEST(ms > 10);

  sm.lock();
  ms = 0;
  std::thread t4([&sm, &ms]() {
    uint64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    sm.lock_shared();
    sm.unlock_shared();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count() - start;
    });
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  sm.try_lock();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  sm.unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(11));
  sm.unlock();
  t4.join();
  BOOST_TEST(ms > 10);
}
BOOST_AUTO_TEST_SUITE_END()
}
