#ifndef LOGICSIM_VOCABULARY_SIZE_DEVICE_H
#define LOGICSIM_VOCABULARY_SIZE_DEVICE_H

#include "format/struct.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Size in device coordinates (scaled by device pixel ratio).
 */
struct size_device_t {
    using value_type = int;

    value_type x;
    value_type y;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const size_device_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const size_device_t &other) const = default;
};

static_assert(std::is_trivial_v<size_device_t>);

}  // namespace logicsim

#endif
