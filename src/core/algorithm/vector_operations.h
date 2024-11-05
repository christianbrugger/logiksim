#ifndef LOGICSIM_CORE_ALGORITHM_VECTOR_OPERATIONS_H
#define LOGICSIM_CORE_ALGORITHM_VECTOR_OPERATIONS_H

#include <gsl/gsl>

#include <optional>
#include <vector>

namespace logicsim {

/**
 * @brief: Get last vector element with bounds checking.
 *
 * Teminates if the vector is empty.
 */
template <typename T>
auto at_back_vector(std::vector<T>& data) {
    Expects(!data.empty());
    return data.back();
}

/**
 * @brief: Get last vector element if it exists.
 */
template <typename T>
auto get_back_vector(std::vector<T>& data) -> std::optional<std::decay_t<T>> {
    if (!data.empty()) {
        return std::make_optional(data.back());
    }
    return std::nullopt;
}

/**
 * @brief: remove and return the last element.
 *
 * Teminates if the vector is empty.
 */
template <typename T>
auto pop_back_vector(std::vector<T>& data) {
    Expects(!data.empty());
    auto entry = std::move(data.back());
    data.pop_back();
    return entry;
}

}  // namespace logicsim

#endif
