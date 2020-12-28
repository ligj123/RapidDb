#pragma once
#include <atomic>
#include <thread>
#include <cassert>

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

  class SharedSpinMutex {
  protected:
    std::atomic<bool> _readFlag{ false };
    std::atomic<bool> _writeFlag{ false };
    volatile int32_t _readCount = 0;
    hash<std::thread::id> thread_hasher;
  public:
    SharedSpinMutex() = default;
    SharedSpinMutex(const SharedSpinMutex&) = delete;
    SharedSpinMutex& operator= (const SharedSpinMutex&) = delete;
    void lock() {
      while (_writeFlag.exchange(true, std::memory_order_acquire)) {
        std::this_thread::yield();
      }

      while (_readCount != 0) {
        std::this_thread::yield();
      }
    }

    void unlock() {
      _writeFlag.store(false, std::memory_order_release);
    }

    void lock_shared()
    {
      size_t currId = thread_hasher(this_thread::get_id());
      while (_readFlag.exchange(true, std::memory_order_relaxed))
      {
        std::this_thread::yield();
      }
      while (true)
      {
        if (!_writeFlag.load(memory_order_relaxed)) break;
        std::this_thread::yield();
      }

      _readCount++;
      _readFlag.store(false, std::memory_order_relaxed);
    }

    void unlock_shared() {
      while (_readFlag.exchange(true, std::memory_order_relaxed))
      {
        std::this_thread::yield();
      }
      _readCount--;
      _readFlag.store(false, std::memory_order_relaxed);
    }
  };

  class RecursiveSpinMutex {
  protected:
    std::atomic<bool> _flag = ATOMIC_VAR_INIT(false);
    size_t _owner = 0;
    int32_t _rcvCount = 0;
    hash<std::thread::id> thread_hasher;
  public:
    RecursiveSpinMutex() = default;
    RecursiveSpinMutex(const RecursiveSpinMutex&) = delete;
    RecursiveSpinMutex& operator= (const RecursiveSpinMutex&) = delete;
    inline void lock() {
      size_t currId = thread_hasher(this_thread::get_id());
      if (_owner == currId)
      {
        _rcvCount++;
        return;
      }
      for (;;) {
        if (!_flag.exchange(true, std::memory_order_acquire))
        {
          assert(_rcvCount == 0);
          _owner = currId;
          break;
        }
        else if (_owner == currId)
        {
          assert(_rcvCount > 0);
          break;
        }

        while (_flag.load(std::memory_order_relaxed)) {
          std::this_thread::yield();
        }
      }

      _rcvCount++;
    }

    bool try_lock() noexcept {
      size_t currId = thread_hasher(this_thread::get_id());
      if (_owner == currId)
      {
        _rcvCount++;
        return true;
      }

      if (!_flag.load(std::memory_order_relaxed) && !_flag.exchange(true, std::memory_order_acquire))
      {
        _rcvCount++;
        return true;
      }

      return false;
    }

    inline void unlock() {
      assert(_owner == thread_hasher(this_thread::get_id()));
      _rcvCount--;
      if (_rcvCount == 0)
      {
        _owner = 0;
        _flag.store(false, std::memory_order_release);
      }
    }
  };
}