#ifndef LOGICSIM_CORE_ALGORITHM_SPAN_OPERATIONS_H
#define LOGICSIM_CORE_ALGORITHM_SPAN_OPERATIONS_H

#include <optional>
#include <span>
#include <stdexcept>

namespace logicsim {

template <typename T>
[[nodiscard]] constexpr auto checked_at(const std::span<T>& data,
                                        typename std::span<T>::size_type pos)
    -> std::span<T>::reference {
    if (pos >= data.size()) {
        throw std::out_of_range {"span pos >= size"};
    }
    return data[pos];
}

/**
 * @brief: Get last span element if it exists.
 */
template <typename T>
auto get_back_span(const std::span<T>& data) -> std::optional<std::decay_t<T>> {
    if (!data.empty()) {
        return std::make_optional(data.back());
    }
    return std::nullopt;
}

}  // namespace logicsim

#endif
