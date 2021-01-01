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

  protected:
    std::atomic<bool> _flag = ATOMIC_VAR_INIT(false);
  };

  class SharedSpinMutex {
  public:
    SharedSpinMutex() = default;
    SharedSpinMutex(const SharedSpinMutex&) = delete;
    SharedSpinMutex& operator= (const SharedSpinMutex&) = delete;

    void lock() noexcept {
      while (_writeFlag.exchange(true, std::memory_order_acquire)) {
        std::this_thread::yield();
      }

      while (_readCount.load(std::memory_order_relaxed) > 0) {
        std::this_thread::yield();
      }
    }

    bool try_lock() noexcept {
      if (_readCount.load(std::memory_order_relaxed) == 0 &&
        !_writeFlag.exchange(true, std::memory_order_acquire)) {
        if (_readCount.load(std::memory_order_relaxed) > 0) {
          _writeFlag.store(false, std::memory_order_relaxed);
          return false;
        }

        return true;
      }

      return false;
    }

    void unlock() noexcept {
      _writeFlag.store(false, std::memory_order_release);
    }

    void lock_shared() noexcept {
      while (true)
      {
        _readCount.fetch_add(1, std::memory_order_acquire);
        if (!_writeFlag.load(std::memory_order_relaxed)) break;

        _readCount.fetch_sub(1, std::memory_order_relaxed);
        std::this_thread::yield();
      }
    }

    bool try_lock_shared() noexcept {
      if (!_writeFlag.load(std::memory_order_relaxed)) {
        _readCount.fetch_add(1, std::memory_order_acquire);
        if (_writeFlag.load(std::memory_order_relaxed)) {
          _readCount.fetch_sub(1, std::memory_order_relaxed);
          return false;
        }

        return true;
      }

      return false;
    }

    void unlock_shared() noexcept {
      _readCount.fetch_sub(1, std::memory_order_relaxed);
    }
  protected:
    std::atomic<uint32_t> _readCount{ 0 };
    std::atomic<bool> _writeFlag{ false };
  };

  class RecursiveSpinMutex {
  protected:
    std::atomic<bool> _flag = ATOMIC_VAR_INIT(false);
    size_t _owner = 0;
    int32_t _rcvCount = 0;
    std::hash<std::thread::id> thread_hasher;
  public:
    RecursiveSpinMutex() = default;
    RecursiveSpinMutex(const RecursiveSpinMutex&) = delete;
    RecursiveSpinMutex& operator= (const RecursiveSpinMutex&) = delete;

    inline void lock() noexcept {
      size_t currId = thread_hasher(std::this_thread::get_id());
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
      size_t currId = thread_hasher(std::this_thread::get_id());
      if (_owner == currId)
      {
        _rcvCount++;
        return true;
      }

      if (!_flag.load(std::memory_order_relaxed) && !_flag.exchange(true, std::memory_order_acquire))
      {
        _owner = currId;
        _rcvCount++;
        return true;
      }

      return false;
    }

    inline void unlock() noexcept {
      assert(_owner == thread_hasher(std::this_thread::get_id()));
      _rcvCount--;
      if (_rcvCount == 0)
      {
        _owner = 0;
        _flag.store(false, std::memory_order_release);
      }
    }
  };
}