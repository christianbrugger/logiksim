#ifndef LOGICSIM_ITERATOR_ADAPTOR_ENUMERATE_H
#define LOGICSIM_ITERATOR_ADAPTOR_ENUMERATE_H

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>  // std::pair, std::forward

namespace logicsim {

/*
template <std::ranges::input_range T>
constexpr auto enumerate(T &&range) {
    struct Iterator {
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;

        using difference_type = std::ranges::range_difference_t<T>;
        using value_type = std::pair<difference_type, std::ranges::range_value_t<T>>;
        using reference = std::pair<difference_type, std::ranges::range_reference_t<T>>;
        using pointer = void;

        std::ranges::iterator_t<T> iterator {};
        difference_type index {};

        auto operator++() -> Iterator & {
            ++index;
            ++iterator;
            return *this;
        }

        auto operator++(int) -> Iterator {
            auto tmp = *this;
            operator++();
            return tmp;
        }

        auto operator*() const -> reference {
            return reference {index, *iterator};
        }

        auto operator==(const Iterator &other) const -> bool {
            return iterator == other.iterator;
        }
    };

    struct View {
        T iterable;

        auto begin() {
            return Iterator {std::begin(iterable)};
        }

        auto end() {
            return Iterator {std::end(iterable)};
        }
    };

    static_assert(std::forward_iterator<Iterator>);
    static_assert(std::ranges::forward_range<View>);

    return View {std::forward<T>(range)};
}
*/

}

#endif
