#ifndef CIRCUIT_VOCABULARY_H
#define CIRCUIT_VOCABULARY_H

#include "exceptions.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <chrono>
#include <compare>
#include <cstdint>
#include <type_traits>

namespace logicsim {

//
// Circuit Types
//

struct circuit_id_t {
    using value_type = int16_t;
    value_type value;

    [[nodiscard]] auto operator==(const circuit_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const circuit_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };
};

static_assert(std::is_trivial<circuit_id_t>::value);

struct element_id_t {
    using value_type = int32_t;
    value_type value;

    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };
};

static_assert(std::is_trivial<element_id_t>::value);

struct connection_id_t {
    using value_type = int8_t;
    value_type value;

    [[nodiscard]] auto operator==(const connection_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };
};

static_assert(std::is_trivial<connection_id_t>::value);

inline constexpr auto null_element = element_id_t {-1};
inline constexpr auto null_connection = connection_id_t {-1};

//
// Time Types
//

#ifdef _MSC_VER
#pragma warning(push)
// false-positive: literal suffix identifiers are reserved
#pragma warning(disable : 4455)
#endif
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct time_t {
    using value_type = std::chrono::duration<int64_t, std::nano>;
    using rep = value_type::rep;
    value_type value;

    [[nodiscard]] auto operator==(const time_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_t &other) const = default;

    [[nodiscard]] static constexpr auto zero() noexcept -> time_t {
        return time_t {value_type::zero()};
    };

    [[nodiscard]] static constexpr auto epsilon() noexcept -> time_t {
        return time_t {++value_type::zero()};
    };

    [[nodiscard]] static constexpr auto min() noexcept -> time_t {
        return time_t {value_type::min()};
    };

    [[nodiscard]] static constexpr auto max() noexcept -> time_t {
        return time_t {value_type::max()};
    };
};

static_assert(std::is_trivial<time_t>::value);

struct delay_t {
    std::chrono::duration<int32_t, std::nano> value {};

    [[nodiscard]] constexpr explicit delay_t() noexcept = default;

    [[nodiscard]] constexpr explicit delay_t(
        std::chrono::duration<int64_t, std::nano> delay)
        : value {delay} {
        if (value != delay) {
            throw_exception("delay cannot be represented.");
        }
    };

    [[nodiscard]] auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const delay_t &other) const = default;
};

//
// Styling Types
//

struct color_t {
    using value_type = uint32_t;
    value_type value;

    [[nodiscard]] auto operator==(const color_t &other) const -> bool = default;

    [[nodiscard]] constexpr auto r() const noexcept -> int {
        return (value >> 16) & 0xFFu;
    }

    [[nodiscard]] constexpr auto g() const noexcept -> int {
        return (value >> 8) & 0xFFu;
    }

    [[nodiscard]] constexpr auto b() const noexcept -> int {
        return (value >> 0) & 0xFFu;
    }

    [[nodiscard]] constexpr auto a() const noexcept -> int {
        return (value >> 24);
    }
};

//
// Spacial Types
//

struct grid_t {
    using value_type = int16_t;
    value_type value;

    [[nodiscard]] auto operator==(const grid_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const grid_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept {
        return std::numeric_limits<value_type>::min();
    };

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    // arithmetic (overflow is checked)

    [[nodiscard]] constexpr auto operator+(grid_t other) const -> grid_t {
        auto result = value + other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<grid_t::value_type>(result)};
    }

    [[nodiscard]] constexpr auto operator-(grid_t other) const -> grid_t {
        auto result = value - other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<grid_t::value_type>(result)};
    }

    // conversions

    [[nodiscard]] explicit constexpr operator int() const noexcept {
        return static_cast<int>(value);
    }

    [[nodiscard]] explicit constexpr operator double() const noexcept {
        return static_cast<double>(value);
    }

    [[nodiscard]] friend constexpr auto operator*(grid_t a, double b) -> double {
        return static_cast<double>(a) * b;
    }
};

static_assert(std::is_trivial<grid_t>::value);

struct point_fine_t {
    double x;
    double y;
};

static_assert(std::is_trivial<point_fine_t>::value);

struct point_t {
    grid_t x;
    grid_t y;

    [[nodiscard]] explicit constexpr operator point_fine_t() const noexcept {
        return point_fine_t {static_cast<double>(x), static_cast<double>(y)};
    }

    [[nodiscard]] constexpr auto operator==(const point_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_t &other) const = default;
};

static_assert(std::is_trivial<point_t>::value);

constexpr auto is_orthogonal(point_t p0, point_t p1) noexcept -> bool {
    // xor to disallow zero length
    return (p0.x == p1.x) ^ (p0.y == p1.y);
}

// a line is either horizontal or vertical
struct line_t {
    point_t p0;
    point_t p1;

    [[nodiscard]] explicit constexpr line_t() noexcept : p0 {}, p1 {} {};

    [[nodiscard]] explicit constexpr line_t(point_t p0_, point_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (!is_orthogonal(p0_, p1_)) [[unlikely]] {
            throw_exception("line needs to be horizontal or vertical.");
        }
    };

    [[nodiscard]] constexpr auto operator==(const line_t &other) const -> bool = default;
};

static_assert(std::is_trivially_copyable<line_t>::value);
static_assert(std::is_trivially_copy_assignable<line_t>::value);

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::circuit_id_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::circuit_id_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::element_id_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::element_id_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::connection_id_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::connection_id_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.value);
    }
};

namespace logicsim {
template <typename OutputIt, typename T>
auto format_microsecond_time(OutputIt out, T time_value) {
    if (-1us < time_value && time_value < 1us) {
        return fmt::format_to(out, "{}ns", time_value.count());
    }
    auto time_us = std::chrono::duration<double, std::micro> {time_value};
    return fmt::format_to(out, "{:L}us", time_us.count());
}
}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::time_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::time_t &obj, fmt::format_context &ctx) {
        return logicsim::format_microsecond_time(ctx.out(), obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::delay_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::delay_t &obj, fmt::format_context &ctx) {
        return logicsim::format_microsecond_time(ctx.out(), obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::color_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::color_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{:X}", obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::grid_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::grid_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::point_fine_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::point_fine_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{:.3f}, {:.3f}]", obj.x, obj.y);
    }
};

template <>
struct fmt::formatter<logicsim::point_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::point_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x, obj.y);
    }
};

template <>
struct fmt::formatter<logicsim::line_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::line_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "Line({}, {})", obj.p0, obj.p1);
    }
};

#endif