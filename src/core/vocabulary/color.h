#ifndef LOGICSIM_VOCABULARY_COLOR_H
#define LOGICSIM_VOCABULARY_COLOR_H

#include "core/format/struct.h"

#include <compare>
#include <cstdint>
#include <type_traits>

struct BLRgba32;

namespace logicsim {

/**
 * @brief: A 32-bit RGBA color value.
 */
struct color_t {
    using value_type = uint32_t;
    // we expose the value, as the type has no invariant
    value_type value {0};

    [[nodiscard]] explicit constexpr color_t() = default;
    [[nodiscard]] explicit constexpr color_t(uint32_t value_) noexcept;
    [[nodiscard]] explicit constexpr color_t(uint32_t r, uint32_t g, uint32_t b,
                                             uint32_t a = 0xFFu) noexcept;

    // we make this operator implicit so we can pass color_t to the Blend2D API
    [[nodiscard]] /* non-explicit */ operator BLRgba32() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const color_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const color_t &other) const = default;

    [[nodiscard]] constexpr auto r() const noexcept -> uint32_t;
    [[nodiscard]] constexpr auto g() const noexcept -> uint32_t;
    [[nodiscard]] constexpr auto b() const noexcept -> uint32_t;
    [[nodiscard]] constexpr auto a() const noexcept -> uint32_t;
};

static_assert(std::is_trivially_copyable_v<color_t>);
static_assert(std::is_trivially_copy_assignable_v<color_t>);

//
// Free Functions
//

constexpr auto monochrome(uint8_t value) -> color_t;
constexpr auto is_rgb(color_t value) -> bool;

//
// Implementation
//

constexpr color_t::color_t(uint32_t value_) noexcept : value {value_} {}

constexpr color_t::color_t(uint32_t r, uint32_t g, uint32_t b, uint32_t a) noexcept
    // TODO: change number type to interger-like
    // TODO: check 0 <= value <= 255
    : value((r << 16) | (g << 8) | b | (a << 24)) {}

constexpr auto color_t::r() const noexcept -> uint32_t {
    return (value >> 16) & 0xFFu;
}

constexpr auto color_t::g() const noexcept -> uint32_t {
    return (value >> 8) & 0xFFu;
}

constexpr auto color_t::b() const noexcept -> uint32_t {
    return (value >> 0) & 0xFFu;
}

constexpr auto color_t::a() const noexcept -> uint32_t {
    return (value >> 24);
}

//
// Free Functions
//

constexpr auto monochrome(uint8_t value) -> color_t {
    return color_t {value, value, value, 255};
}

constexpr auto is_rgb(color_t value) -> bool {
    return value.a() == 255;
}

//
// Constants
//

namespace defaults {
constexpr static inline auto color_transparent_black = color_t {0x00000000};
constexpr static inline auto color_transparent_white = color_t {0x00FFFFFF};

constexpr static inline auto color_black = color_t {0xFF000000};
constexpr static inline auto color_white = color_t {0xFFFFFFFF};

constexpr static inline auto color_gray = color_t {0xFF808080};
constexpr static inline auto color_light_gray = color_t {0xFFD3D3D3};
constexpr static inline auto color_gray_90 = color_t {0xFFE5E5E5};

constexpr static inline auto color_red = color_t {0xFFFF0000};
constexpr static inline auto color_green = color_t {0xFF009900};
constexpr static inline auto color_dark_green = color_t {0xFF006400};
constexpr static inline auto color_lime = color_t {0xFF00FF00};
constexpr static inline auto color_yellow = color_t {0xFFFFFF00};
constexpr static inline auto color_orange = color_t {0xFFFF8000};
constexpr static inline auto color_dark_orange = color_t {0xFFB36200};
constexpr static inline auto color_blue = color_t {0xFF0000FF};
constexpr static inline auto color_cyan = color_t {0xFF00FFFF};
constexpr static inline auto color_light_blue = color_t {0xFF5097DE};
constexpr static inline auto color_dark_blue = color_t {0xFF3D85B8};
}  // namespace defaults

}  // namespace logicsim

#endif
