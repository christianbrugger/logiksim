#ifndef LOGICSIM_TYPE_TRAIT_CONST_ITERATOR_H
#define LOGICSIM_TYPE_TRAIT_CONST_ITERATOR_H

#include <iterator>
#include <ranges>
#include <utility>

namespace logicsim {

/**
 * @brief: Resolves to the const iterator type of the given range
 *
 * Note this is backported from C++23
 */
template <std::ranges::range R>
using const_iterator_t = decltype(std::ranges::cbegin(std::declval<R&>()));

/**
 * @brief: Resolves to the const sentinel type of the given range
 *
 * Note this is backported from C++23
 */
template <std::ranges::range R>
using const_sentinel_t = decltype(std::ranges::end(std::declval<const R&>()));

}  // namespace logicsim

#endif
