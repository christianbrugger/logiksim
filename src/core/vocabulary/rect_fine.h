#ifndef LOGICSIM_VOCABULARY_RECT_FINE_H
#define LOGICSIM_VOCABULARY_RECT_FINE_H

#include "core/format/struct.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/rect.h"

#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A continuous 2-d rect defined by two ordered points p0 and p1.
 *
 * Class invariants:
 *     * both coordinates are ordered:
 *         * p0.x <= p1.x
 *         * p0.y <= p1.y
 */
struct rect_fine_t {
    point_fine_t p0 {};
    point_fine_t p1 {};

    [[nodiscard]] explicit constexpr rect_fine_t() = default;
    [[nodiscard]] explicit constexpr rect_fine_t(rect_t rect) noexcept;
    [[nodiscard]] explicit constexpr rect_fine_t(point_fine_like auto p0_,
                                                 point_fine_like auto p1_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const rect_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const rect_fine_t &other) const = default;

    constexpr auto operator+=(const point_fine_like auto &other) -> rect_fine_t &;
    constexpr auto operator-=(const point_fine_like auto &other) -> rect_fine_t &;
};

static_assert(std::is_trivially_copyable_v<rect_fine_t>);
static_assert(std::is_trivially_copy_assignable_v<rect_fine_t>);

[[nodiscard]] constexpr auto operator+(const rect_fine_t &left,
                                       const point_fine_like auto &right) -> rect_fine_t;
[[nodiscard]] constexpr auto operator-(const rect_fine_t &left,
                                       const point_fine_like auto &right) -> rect_fine_t;
// symmetric
[[nodiscard]] constexpr auto operator+(const point_fine_like auto &left,
                                       const rect_fine_t &right) -> rect_fine_t;

//
// Implementation
//

constexpr rect_fine_t::rect_fine_t(rect_t rect) noexcept
    : p0 {point_fine_t {rect.p0}}, p1 {point_fine_t {rect.p1}} {}

constexpr rect_fine_t::rect_fine_t(point_fine_like auto p0_, point_fine_like auto p1_)
    : p0 {point_fine_t {p0_}}, p1 {point_fine_t {p1_}} {
    if (p0.x > p1.x || p0.y > p1.y) [[unlikely]] {
        throw std::runtime_error("point in rect_t need to be ordered");
    }
}

constexpr auto rect_fine_t::operator+=(const point_fine_like auto &other)
    -> rect_fine_t & {
    p0 += point_fine_t {other};
    p1 += point_fine_t {other};
    return *this;
}

constexpr auto rect_fine_t::operator-=(const point_fine_like auto &other)
    -> rect_fine_t & {
    p0 -= point_fine_t {other};
    p1 -= point_fine_t {other};
    return *this;
}

constexpr auto operator+(const rect_fine_t &left, const point_fine_like auto &right)
    -> rect_fine_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const rect_fine_t &left, const point_fine_like auto &right)
    -> rect_fine_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator+(const point_fine_like auto &left, const rect_fine_t &right)
    -> rect_fine_t {
    return operator+(right, left);
}

}  // namespace logicsim

#endif
