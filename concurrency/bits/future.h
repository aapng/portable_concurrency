#pragma once

#include <future>

#include "fwd.h"

#include "shared_state.h"
#include "utils.h"

namespace concurrency {

namespace detail {

template<typename F, typename T, typename R>
class future_continuation_helper: public continuation {
  static_assert(std::is_same<R, std::result_of_t<F(future<T>)>>::value, "Invalid return type");
public:
  future_continuation_helper(F&& f, future<T>&& future):
    future_(std::move(future)),
    f_(std::forward<F>(f))
  {}

  void invoke() noexcept override try {
    // TODO: state_->emplace(INVOKE(std::move(f_), std::move(future_)));
    // proper INVOKE implementation required
    state_->emplace(f_(std::move(future_)));
  } catch(...) {
    state_->set_exception(std::current_exception());
  }

  auto get_state() {
    return state_;
  }

private:
  future<T> future_;
  std::decay_t<F> f_;
  std::shared_ptr<shared_state<R>> state_ = std::make_shared<shared_state<R>>();
};

template<typename F, typename T>
class future_continuation_helper<F, T, void>: public continuation {
  static_assert(std::is_void<std::result_of_t<F(future<T>)>>::value, "Invalid return type");
public:
  future_continuation_helper(F&& f, future<T>&& future):
    future_(std::move(future)),
    f_(std::forward<F>(f))
  {}

  void invoke() noexcept override try {
    // TODO: state_->emplace(INVOKE(std::move(f_), std::move(future_)));
    // proper INVOKE implementation required
    f_(std::move(future_));
    state_->emplace();
  } catch(...) {
    state_->set_exception(std::current_exception());
  }

  auto get_state() {
    return state_;
  }

private:
  future<T> future_;
  std::decay_t<F> f_;
  std::shared_ptr<shared_state<void>> state_ = std::make_shared<shared_state<void>>();
};

template<typename F, typename T>
using future_continuation = future_continuation_helper<F, T, std::result_of_t<F(future<T>)>>;

} // namespace detail

template<typename T>
class future {
public:
  future() noexcept = default;
  future(future&&) noexcept = default;
  future(const future&) = delete;

  future& operator= (const future&) = delete;
  future& operator= (future&& rhs) noexcept {
    state_ = std::move(rhs.state_);
    rhs.state_ = nullptr;
    return *this;
  }

  ~future() = default;

  shared_future<T> share() noexcept {
    return {std::move(*this)};
  }

  T get() {
    if (!state_)
      throw std::future_error(std::future_errc::no_state);
    auto state = std::move(state_);
    return state->get();
  }

  void wait() const {
    if (!state_)
      throw std::future_error(std::future_errc::no_state);
    state_->wait();
  }

  template<typename Rep, typename Period>
  std::future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const {
    if (!state_)
      throw std::future_error(std::future_errc::no_state);
    return state_->wait_for(rel_time);
  }

  template <typename Clock, typename Duration>
  std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const {
    if (!state_)
      throw std::future_error(std::future_errc::no_state);
    return state_->wait_until(abs_time);
  }

  bool valid() const noexcept {return static_cast<bool>(state_);}

  bool is_ready() const {
    if (!state_)
      throw std::future_error(std::future_errc::no_state);
    return state_->is_ready();
  }

  template<typename F>
  auto then(F&& f) {
    if (!state_)
      throw std::future_error(std::future_errc::no_state);
    auto state = state_;
    auto cont = std::make_shared<detail::future_continuation<F, T>>(
      std::forward<F>(f),
      std::move(*this)
    );
    auto child_state = cont->get_state();
    state->add_continuation(std::move(cont));
    return detail::make_future<std::result_of_t<F(future<T>)>>(std::move(child_state));
  }

private:
  friend class shared_future<T>;
  friend future<T> detail::make_future<T>(std::shared_ptr<detail::shared_state<T>>&& state);

  explicit future(std::shared_ptr<detail::shared_state<T>>&& state) noexcept:
    state_(std::move(state))
  {}

private:
  std::shared_ptr<detail::shared_state<T>> state_;
};

} // namespace concurrency
