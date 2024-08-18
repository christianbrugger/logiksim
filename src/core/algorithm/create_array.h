#ifndef LOGICSIM_ALGORITHM_CREATE_ARRAY_H
#define LOGICSIM_ALGORITHM_CREATE_ARRAY_H

#include <array>
#include <utility>

namespace logicsim {

namespace detail {

template <typename T, std::size_t... Is>
constexpr auto create_array(T value, std::index_sequence<Is...> /*unused*/)
    -> std::array<T, sizeof...(Is)> {
    // cast Is to void to remove the warning: unused value
    return {{(static_cast<void>(Is), value)...}};
}

}  // namespace detail

/**
 * @brief: Construct array with non default constructable values in constexpr context.
 */
template <std::size_t N, typename T>
constexpr auto create_array(const T& value) -> std::array<T, N> {
    return detail::create_array(value, std::make_index_sequence<N>());
}

}  // namespace logicsim

#endif  // LOGICSIM_ALGORITHM_CREATE_ARRAY_H
