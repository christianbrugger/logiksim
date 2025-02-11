#pragma once

#include "main_winui/src/ls_concurrent_queue.h"
#include "main_winui/src/ls_vocabulary.h"

#include <gsl/gsl>

#include <condition_variable>
#include <mutex>
#include <queue>

namespace logicsim {

/**
 * @brief: Blocking fast concurrent queue.
 *
 * Class-invariants:
 *   + shutdown is never set to false, once set to true.
 */
template <typename T>
class ConcurrentBlockingQueue {
   public:
    auto push(const T& value) -> void;
    auto push(T&& value) -> void;

    /**
     * @brief: Returns next queue item.
     *
     * Blocks until an entry is available or shutdown is initiaed.
     *
     * Throws ShutdownException, if queue is shut down.
     */
    auto pop() -> T;

    /**
     * @brief: Returns next queue item or std::nullopt.
     *
     * Throws ShutdownException, if queue is shut down.
     */
    auto try_pop() -> std::optional<T>;

    auto shutdown() -> void;

    [[nodiscard]] auto unsafe_size() const -> std::size_t;

   private:
    mutable std::mutex queue_mutex_ {};
    std::condition_variable queue_cv_ {};

    ConcurrentQueue<T> queue_ {};
    bool shutdown_ {false};
};

//
// Implementation
//

template <typename T>
auto ConcurrentBlockingQueue<T>::push(const T& value) -> void {
    {
        const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);
        queue_.push(value);
    }
    queue_cv_.notify_one();
}

template <typename T>
auto ConcurrentBlockingQueue<T>::push(T&& value) -> void {
    {
        const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);
        queue_.push(std::move(value));
    }
    queue_cv_.notify_one();
}

template <typename T>
auto ConcurrentBlockingQueue<T>::pop() -> T {
    auto lock = std::unique_lock(queue_mutex_);
    queue_cv_.wait(lock, [&]() { return shutdown_ || !queue_.empty(); });

    if (shutdown_) {
        throw ShutdownException {"ConcurrentBlockingQueue shutdown."};
    }

    return queue_.try_pop().value();
}

template <typename T>
auto ConcurrentBlockingQueue<T>::try_pop() -> std::optional<T> {
    const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);

    if (shutdown_) {
        throw ShutdownException {"ConcurrentBlockingQueue shutdown."};
    }

    return queue_.try_pop();
}

template <typename T>
auto ConcurrentBlockingQueue<T>::shutdown() -> void {
    {
        const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);
        shutdown_ = true;
    }
    queue_cv_.notify_all();
}

template <typename T>
auto ConcurrentBlockingQueue<T>::unsafe_size() const -> std::size_t {
    return queue_.unsafe_size();
}

}  // namespace logicsim