#ifndef LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_OUTPUT_ITERATOR_H
#define LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_OUTPUT_ITERATOR_H

#include <functional>  // std::invoke
#include <iterator>
#include <utility>  // std::forward, std::move

namespace logicsim {

template <class Proj, typename Iterator>
class transform_output_iterator {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = typename Iterator::difference_type;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit transform_output_iterator(Proj proj, Iterator iterator)
        : iterator_(std::move(iterator)), proj_(std::move(proj)) {}

    auto operator++() -> transform_output_iterator & {
        ++iterator_;
        return *this;
    }

    auto operator++(int) -> transform_output_iterator {
        auto temp = *this;
        ++iterator_;
        return temp;
    }

    auto operator*() -> transform_output_iterator & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> transform_output_iterator & {
        *iterator_ = std::invoke(proj_, std::forward<T>(value));
        return *this;
    }

   private:
    Iterator iterator_;
    Proj proj_;
};

}  // namespace logicsim

#endif
