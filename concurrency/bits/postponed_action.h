#pragma once

#include <memory>
#include <tuple>
#include <utility>

#include "invoke.h"

namespace experimental {
inline namespace concurrency_v1 {
namespace detail {

class erased_action {
public:
  erased_action() = default;
  virtual ~erased_action() = default;

  erased_action(erased_action&&) = delete;

  virtual void invoke() = 0;
};

class postponed_action {
public:
  postponed_action() = default;

  template<typename F, typename... A>
  postponed_action(F&& f, A&&... a);

  postponed_action(std::unique_ptr<erased_action>&& action):
    action_(std::move(action))
  {}

  void operator() () {
    action_->invoke();
    action_.reset();
  }

  explicit operator bool () const noexcept {return static_cast<bool>(action_);}

private:
  std::unique_ptr<erased_action> action_;
};

template<typename F, typename... A>
struct concrete_action: public erased_action {
  std::decay_t<F> func;
  std::tuple<std::decay_t<A>...> args;

  concrete_action(F&& f, A&&... a):
    func(std::forward<F>(f)),
    args(std::forward<A>(a)...)
  {}

  void invoke() override {
    invoke(std::make_index_sequence<sizeof...(A)>());
  }

private:
  template<size_t... I>
  void invoke(std::index_sequence<I...>) {
    ::experimental::concurrency_v1::detail::invoke(std::move(func), std::move(std::get<I>(args))...);
  }
};

template<typename F, typename... A>
postponed_action::postponed_action(F&& f, A&&... a):
  action_(new concrete_action<F, A...>{std::forward<F>(f), std::forward<A>(a)...})
{}

} // namespace detail
} // inline namespace concurrency_v1
} // namespace experimental
