#pragma once
#include <atomic>
#include <thread>

namespace utils {
  class SpinMutex {
  public:
    SpinMutex() = default;
    SpinMutex(const SpinMutex&) = delete;
    SpinMutex& operator= (const SpinMutex&) = delete;
    inline void lock() noexcept {
      for (;;) {
        if (!_flag.exchange(true, std::memory_order_acquire)) {
          break;
        }
        while (_flag.load(std::memory_order_relaxed)) {
          std::this_thread::yield();
        }
      }
    }

    bool try_lock() noexcept {
      return !_flag.load(std::memory_order_relaxed) &&
        !_flag.exchange(true, std::memory_order_acquire);
    }
    inline void unlock() noexcept {
      _flag.store(false, std::memory_order_release);
    }

  private:
    std::atomic<bool> _flag = ATOMIC_VAR_INIT(false);
  };
}