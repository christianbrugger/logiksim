#ifndef LOGICSIM_CORE_ALGORITHM_AT_BACK_VECTOR_H
#define LOGICSIM_CORE_ALGORITHM_AT_BACK_VECTOR_H

#include <gsl/gsl>

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

}  // namespace logicsim

#endif
