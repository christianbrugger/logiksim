#ifndef LOGICSIM_VOCABULARY_COLOR_H
#define LOGICSIM_VOCABULARY_COLOR_H

#include "format/struct.h"

#include <compare>
#include <cstdint>

struct BLRgba32;

namespace logicsim {

/**
 * @brief: A 32-bit RGBA color value.
 */
struct color_t {
    using value_type = uint32_t;
    value_type value;

    color_t() = default;

    constexpr explicit color_t(uint32_t value_) noexcept : value {value_} {}

    constexpr explicit color_t(uint32_t r, uint32_t g, uint32_t b,
                               uint32_t a = 0xFFu) noexcept
        : value((r << 16) | (g << 8) | b | (a << 24)) {}

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const color_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const color_t &other) const = default;

    [[nodiscard]] constexpr auto r() const noexcept -> uint32_t {
        return (value >> 16) & 0xFFu;
    }

    [[nodiscard]] constexpr auto g() const noexcept -> uint32_t {
        return (value >> 8) & 0xFFu;
    }

    [[nodiscard]] constexpr auto b() const noexcept -> uint32_t {
        return (value >> 0) & 0xFFu;
    }

    [[nodiscard]] constexpr auto a() const noexcept -> uint32_t {
        return (value >> 24);
    }

    [[nodiscard]] operator BLRgba32() const noexcept;
};

namespace defaults {
constexpr static inline auto no_color = ::logicsim::color_t {0x00000000};

constexpr static inline auto color_black = ::logicsim::color_t {0xFF000000};
constexpr static inline auto color_white = ::logicsim::color_t {0xFFFFFFFF};

constexpr static inline auto color_gray = ::logicsim::color_t {0xFF808080};
constexpr static inline auto color_light_gray = ::logicsim::color_t {0xFFD3D3D3};
constexpr static inline auto color_gray_90 = ::logicsim::color_t {0xFFE5E5E5};

constexpr static inline auto color_red = ::logicsim::color_t {0xFFFF0000};
constexpr static inline auto color_green = ::logicsim::color_t {0xFF009900};
constexpr static inline auto color_lime = ::logicsim::color_t {0xFF00FF00};
constexpr static inline auto color_yellow = ::logicsim::color_t {0xFFFFFF00};
constexpr static inline auto color_orange = ::logicsim::color_t {0xFFFF8000};
constexpr static inline auto color_dark_orange = ::logicsim::color_t {0xFFB36200};
constexpr static inline auto color_blue = ::logicsim::color_t {0xFF0000FF};
constexpr static inline auto color_cyan = ::logicsim::color_t {0xFF00FFFF};
constexpr static inline auto color_light_blue = color_t {0xFF5097DE};
constexpr static inline auto color_dark_blue = color_t {0xFF3D85B8};
}  // namespace defaults

}  // namespace logicsim

#endif
