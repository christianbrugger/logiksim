#ifndef LOGICSIM_ITERATOR_ADAPTOR_FILTER_OUTPUT_ITERATOR_H
#define LOGICSIM_ITERATOR_ADAPTOR_FILTER_OUTPUT_ITERATOR_H

#include <functional>  // std::invoke
#include <iterator>
#include <utility>  // std::forward, std::move

namespace logicsim {

template <typename Iterator, class Pred>
class filter_output_iterator {
   public:
    using iterator_concept = std::output_iterator_tag;
    using iterator_category = std::output_iterator_tag;

    using difference_type = typename Iterator::difference_type;
    using value_type = void;
    using pointer = void;
    using reference = void;

    explicit filter_output_iterator(Pred pred, Iterator iterator)
        : iterator_(std::move(iterator)), pred_(std::move(pred)) {}

    auto operator++() -> filter_output_iterator & {
        ++iterator_;
        return *this;
    }

    auto operator++(int) -> filter_output_iterator {
        auto temp = *this;
        ++iterator_;
        return temp;
    }

    auto operator*() -> filter_output_iterator & {
        return *this;
    }

    template <typename T>
    auto operator=(T &&value) -> filter_output_iterator & {
        if (std::invoke(pred_, value)) {
            *iterator_ = std::forward<T>(value);
        }
        return *this;
    }

   private:
    Iterator iterator_;
    Pred pred_;
};

}  // namespace logicsim

#endif
