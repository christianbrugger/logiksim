#ifndef LOGICSIM_CORE_ALGORITHM_POP_BACK_VECTOR_H
#define LOGICSIM_CORE_ALGORITHM_POP_BACK_VECTOR_H

#include <gsl/gsl>

#include <vector>

namespace logicsim {

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
