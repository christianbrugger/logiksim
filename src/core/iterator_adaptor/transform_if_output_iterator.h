#ifndef LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_IF_OUTPUT_ITERATOR_H
#define LOGICSIM_ITERATOR_ADAPTOR_TRANSFORM_IF_OUTPUT_ITERATOR_H

#include <functional>  // std::invoke
#include <iterator>
#include <utility>  // std::forward, std::move

namespace logicsim {

template <typename Pred, class Proj, class Iterator>
class transform_if_output_iterator {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = typename Iterator::difference_type;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit transform_if_output_iterator(Pred pred, Proj proj, Iterator iterator)
        : iterator_(std::move(iterator)),
          proj_(std::move(proj)),
          pred_(std::move(pred)) {}

    auto operator++() -> transform_if_output_iterator & {
        ++iterator_;
        return *this;
    }

    auto operator++(int) -> transform_if_output_iterator {
        auto temp = *this;
        ++iterator_;
        return temp;
    }

    auto operator*() -> transform_if_output_iterator & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> transform_if_output_iterator & {
        if (pred_(value)) {
            *iterator_ = std::invoke(proj_, std::forward<T>(value));
        }
        return *this;
    }

   private:
    Iterator iterator_;
    Proj proj_;
    Pred pred_;
};

}  // namespace logicsim

#endif
