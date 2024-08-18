#ifndef LOGICSIM_ALGORITHM_CHECKED_DEREF_H
#define LOGICSIM_ALGORITHM_CHECKED_DEREF_H

#include <stdexcept>

namespace logicsim {

template <typename T>
[[nodiscard]] constexpr auto checked_deref(T* pointer) -> T& {
    if (pointer == nullptr) [[unlikely]] {
        throw std::runtime_error("Pointer is expected to be null");
    }
    return *pointer;
}

}  // namespace logicsim

#endif
