#ifndef LOGICSIM_CORE_ALGORITHM_SPAN_OPERATIONS_H
#define LOGICSIM_CORE_ALGORITHM_SPAN_OPERATIONS_H

#include <span>
#include <stdexcept>

namespace logicsim {

template <typename T>
[[nodiscard]] constexpr auto checked_at(const std::span<T> &data, std::size_t pos) {
    if (pos >= data.size()) {
        throw std::out_of_range {"span pos >= size"};
    }
    return data[pos];
}

}  // namespace logicsim

#endif
