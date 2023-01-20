#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "exceptions.h"

#include <fmt/core.h>

#include <cassert>
#include <cstdint>

namespace logicsim {

// remove 2d from name
struct point2d_fine_t {
    double x;
    double y;
};

using grid_t = int16_t;

struct point2d_t {
    grid_t x;
    grid_t y;

    explicit operator point2d_fine_t() const noexcept {
        return point2d_fine_t {static_cast<double>(x), static_cast<double>(y)};
    }

    auto operator==(point2d_t other) const noexcept {
        return x == other.x && y == other.y;
    }
};

// TODO create type that is only horizontal or vertical
struct line2d_t {
    point2d_t p0;
    point2d_t p1;

    auto operator==(line2d_t other) const noexcept {
        return p0 == other.p0 && p1 == other.p1;
    }
};

constexpr auto is_orthogonal(point2d_t p0, point2d_t p1) noexcept {
    return p0.x == p1.x || p0.y == p1.y;
}

constexpr auto is_orthogonal(line2d_t line) noexcept -> bool {
    return is_orthogonal(line.p0, line.p1);
}

// line that is either horizontal or vertical
struct orthogonal_line_t {
    point2d_t p0;
    point2d_t p1;

    explicit constexpr orthogonal_line_t() noexcept : p0 {}, p1 {} {};

    explicit constexpr orthogonal_line_t(point2d_t p0_, point2d_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (!is_orthogonal(p0_, p1_)) [[unlikely]] {
            throw_exception("orthogonal line needs to be horizontal or vertical.");
        }
    };

    explicit constexpr orthogonal_line_t(grid_t x0, grid_t y0, grid_t x1, grid_t y1)
        : orthogonal_line_t {{x0, y0}, {x1, y1}} {};

    auto operator==(orthogonal_line_t other) const noexcept {
        return p0 == other.p0 && p1 == other.p1;
    }
};

constexpr auto is_horizontal(orthogonal_line_t line) noexcept -> bool {
    return line.p0.x == line.p0.y;
}

constexpr auto is_vertical(orthogonal_line_t line) noexcept -> bool {
    return !is_horizontal(line);
}

// fast distance for horitonal or vertical lines
inline auto distance_1d(point2d_t p0, point2d_t p1) -> int {
    auto dx = p1.x - p0.x;
    auto dy = p1.y - p0.y;
    assert(dx == 0 || dy == 0);
    return std::abs((dx == 0) ? dy : dx);
}

// fast distance for horitonal or vertical lines
inline auto distance_1d(line2d_t line) -> int {
    return distance_1d(line.p0, line.p1);
}

}  // namespace logicsim

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