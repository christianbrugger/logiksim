#ifndef LOGICSIM_FORMAT_POINTER_H
#define LOGICSIM_FORMAT_POINTER_H

#include <fmt/core.h>

#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Formats a pointer either as nullptr or its formatted value.
 */
template <typename T>
    requires std::is_pointer_v<T>
auto fmt_ptr(T pointer) -> std::string {
    if (pointer == nullptr) {
        return "nullptr";
    }
    return fmt::format("{}", *pointer);
}

}  // namespace logicsim

#endif
