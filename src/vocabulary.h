#ifndef CIRCUIT_VOCABULARY_H
#define CIRCUIT_VOCABULARY_H

#include "exceptions.h"

#include <fmt/core.h>

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

    auto operator==(const circuit_id_t &other) const -> bool = default;
    auto operator<=>(const circuit_id_t &other) const = default;

    static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };
};

static_assert(std::is_trivial<circuit_id_t>::value);

struct element_id_t {
    using value_type = int32_t;
    value_type value;

    auto operator==(const element_id_t &other) const -> bool = default;
    auto operator<=>(const element_id_t &other) const = default;

    static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };
};

static_assert(std::is_trivial<element_id_t>::value);

struct connection_id_t {
    using value_type = int8_t;
    value_type value;

    auto operator==(const connection_id_t &other) const -> bool = default;
    auto operator<=>(const connection_id_t &other) const = default;

    static constexpr auto max() noexcept {
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
#pragma warning(disable : 4455)  // literal suffix identifiers are reserved
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

    auto operator==(const time_t &other) const -> bool = default;
    auto operator<=>(const time_t &other) const = default;

    static constexpr auto zero() noexcept -> time_t {
        return time_t {value_type::zero()};
    };

    static constexpr auto epsilon() noexcept -> time_t {
        return time_t {++value_type::zero()};
    };

    static constexpr auto min() noexcept -> time_t {
        return time_t {value_type::min()};
    };

    static constexpr auto max() noexcept -> time_t {
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

    auto operator==(const delay_t &other) const -> bool = default;
    auto operator<=>(const delay_t &other) const = default;
};

// change to delay_t ?
struct history_t {
    std::chrono::duration<int32_t, std::nano> value {};

    [[nodiscard]] constexpr explicit history_t() noexcept = default;

    [[nodiscard]] constexpr explicit history_t(
        std::chrono::duration<int64_t, std::nano> delay)
        : value {delay} {
        if (value != delay) {
            throw_exception("delay cannot be represented.");
        }
    }

    auto operator==(const history_t &other) const -> bool = default;
    auto operator<=>(const history_t &other) const = default;
};

//
// Spacial Types
//

// TODO strong type
using grid_t = int16_t;

// TODO remove 2d from name
struct point2d_fine_t {
    double x;
    double y;
};

// TODO remove 2d from name
struct point2d_t {
    grid_t x;
    grid_t y;

    explicit constexpr operator point2d_fine_t() const noexcept {
        return point2d_fine_t {static_cast<double>(x), static_cast<double>(y)};
    }

    constexpr auto operator==(const point2d_t &other) const -> bool = default;
    constexpr auto operator<=>(const point2d_t &other) const = default;
};

// TODO remove 2d from name
struct line2d_t {
    point2d_t p0;
    point2d_t p1;

    constexpr auto operator==(const line2d_t &other) const -> bool = default;
};

// line that is either horizontal or vertical
struct orthogonal_line_t {
    point2d_t p0;
    point2d_t p1;

    explicit constexpr orthogonal_line_t() noexcept : p0 {}, p1 {} {};

    explicit constexpr orthogonal_line_t(point2d_t p0_, point2d_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (p0_.x != p1_.x && p0_.y != p1_.y) [[unlikely]] {
            throw_exception("orthogonal line needs to be horizontal or vertical.");
        }
    };

    explicit constexpr orthogonal_line_t(grid_t x0, grid_t y0, grid_t x1, grid_t y1)
        : orthogonal_line_t {{x0, y0}, {x1, y1}} {};

    auto operator==(const orthogonal_line_t &other) const -> bool = default;
};

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
struct fmt::formatter<logicsim::history_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::history_t &obj, fmt::format_context &ctx) {
        return logicsim::format_microsecond_time(ctx.out(), obj.value);
    }
};

template <>
struct fmt::formatter<logicsim::point2d_fine_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::point2d_fine_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{:.3f}, {:.3f}]", obj.x, obj.y);
    }
};

template <>
struct fmt::formatter<logicsim::point2d_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::point2d_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x, obj.y);
    }
};

template <>
struct fmt::formatter<logicsim::line2d_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::line2d_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "Line({}, {})", obj.p0, obj.p1);
    }
};

#endif