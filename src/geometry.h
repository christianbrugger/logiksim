#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

namespace logicsim {

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

#endif