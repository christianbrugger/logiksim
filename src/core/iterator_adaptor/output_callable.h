#ifndef LOGICSIM_ITERATOR_ADAPTOR_OUTPUT_CALLABLE_H
#define LOGICSIM_ITERATOR_ADAPTOR_OUTPUT_CALLABLE_H

#include <functional>  // std::invoke
#include <iterator>
#include <utility>  // std::forward, std::move

namespace logicsim {

/**
 * @brief: Output iterator that calls a function.
 *
 * Note, lambdas with capture are not move-assignable and when used
 * directly with output_callable([..](..){..}) will not fullfill the iterator concept.
 *
 * In this case use std::ref and assign the lambda to a variable.
 *     ... output_callable(std::ref(func)) ...
 *
 *  See: https://stackoverflow.com/questions/35392919
 */
template <class Func>
class output_callable {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = std::ptrdiff_t;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit output_callable(Func func) : func_(std::move(func)) {}

    auto operator++() -> output_callable & {
        return *this;
    }

    auto operator++(int) -> output_callable {
        return *this;
    }

    auto operator*() -> output_callable & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> output_callable & {
        std::invoke(func_, std::forward<T>(value));
        return *this;
    }

   private:
    Func func_;
};

}  // namespace logicsim

#endif
