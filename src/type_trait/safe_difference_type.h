#ifndef LOGICSIM_TYPE_TRAIT_SAFE_DIFFERENCE_TYPE_H
#define LOGICSIM_TYPE_TRAIT_SAFE_DIFFERENCE_TYPE_H

#include <iterator>
#include <type_traits>

namespace logicsim {
/**
 * @brief: A safe difference type of underlying type.
 *
 * If T is not integral, define difference_type in your custom type.
 * In that case this type will be returned.
 *
 */
template <typename T>
using safe_difference_t =
    std::conditional_t<std::is_integral_v<T>,
                       std::conditional_t<sizeof(T) < sizeof(int), int, long long>,
                       std::iter_difference_t<T>>;

}  // namespace logicsim

#endif
