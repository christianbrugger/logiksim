#ifndef LOGICSIM_VOCABULARY_SIZE_DEVICE_H
#define LOGICSIM_VOCABULARY_SIZE_DEVICE_H

#include "core/format/struct.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Size in device coordinates (scaled by device pixel ratio).
 */
struct size_device_t {
    using value_type = int;

    value_type width {0};
    value_type height {0};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const size_device_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const size_device_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<size_device_t>);
static_assert(std::is_trivially_copy_assignable_v<size_device_t>);

}  // namespace logicsim

#endif
