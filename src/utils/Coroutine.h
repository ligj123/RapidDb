#pragma once
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace storage {
template <typename T> struct coro_ret {
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;
  handle_type coro_handle_;

  coro_ret(handle_type h) : coro_handle_(h) {}
  coro_ret(const coro_ret &) = delete;
  coro_ret(coro_ret &&s) : coro_handle_(s.coro_) { s.coro_handle_ = nullptr; }
  ~coro_ret() {
    if (coro_handle_)
      coro_handle_.destroy();
  }
  coro_ret &operator=(const coro_ret &) = delete;
  coro_ret &operator=(coro_ret &&s) {
    coro_handle_ = s.coro_handle_;
    s.coro_handle_ = nullptr;
    return *this;
  }

  bool move_next() {
    coro_handle_.resume();
    return coro_handle_.done();
  }
  T get() { return coro_handle_.promise().return_data_; }
  struct promise_type {
    promise_type() = default;
    ~promise_type() = default;

    auto get_return_object() {
      return coro_ret<T>{handle_type::from_promise(*this)};
    }

    auto initial_suspend() {
      return std::suspend_always{};
    }
    void return_value(T v) {
      return_data_ = v;
      return;
    }
    auto yield_value(T v) {
      return_data_ = v;
      return std::suspend_always{};
    }
    auto final_suspend() noexcept {
      return std::suspend_always{};
    }
    void unhandled_exception() { std::exit(1); }
    T return_data_;
  };
};
} // namespace storage