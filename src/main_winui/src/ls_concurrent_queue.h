#pragma once

#include <concurrent_queue.h>

#include <optional>

namespace logicsim {

/**
 * @brief: Non-blocking fast concurrent queue.
 *
 * Purpose:
 *   + use std::optional in try_pop
 *   + wrap microsoft queue
 */
template <typename T>
class ConcurrentQueue {
   public:
    auto push(const T& value) -> void;
    auto push(T&& value) -> void;
    auto try_pop() -> std::optional<T>;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto unsafe_size() const -> std::size_t;

   private:
    concurrency::concurrent_queue<T> queue_ {};
};

static_assert(std::is_default_constructible_v<ConcurrentQueue<int>>);

//
// Implementation
//

template <typename T>
auto ConcurrentQueue<T>::push(const T& value) -> void {
    queue_.push(value);
}

template <typename T>
auto ConcurrentQueue<T>::push(T&& value) -> void {
    queue_.push(std::move(value));
}

template <typename T>
auto ConcurrentQueue<T>::try_pop() -> std::optional<T> {
    // construct return value exactly for return-value-optimization to work
    // SEE: https://godbolt.org/z/6abaq8d5v

    auto task = std::optional<T> {T {}};

    if (!queue_.try_pop(task.value())) {
        // reset the optional so there is only one return path
        task.reset();
    }

    return task;
}

template <typename T>
auto ConcurrentQueue<T>::empty() const -> bool {
    return queue_.empty();
}

template <typename T>
auto ConcurrentQueue<T>::unsafe_size() const -> std::size_t {
    return queue_.unsafe_size();
}

}  // namespace logicsim