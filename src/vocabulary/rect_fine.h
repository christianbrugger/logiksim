#ifndef LOGICSIM_VOCABULARY_RECT_FINE_H
#define LOGICSIM_VOCABULARY_RECT_FINE_H

#include "format/struct.h"
#include "vocabulary/point.h"
#include "vocabulary/point_fine.h"
#include "vocabulary/rect.h"

#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A continuous 2-d rect defined by two ordered points p0 and p1.
 */
struct rect_fine_t {
    point_fine_t p0;
    point_fine_t p1;

    [[nodiscard]] explicit constexpr rect_fine_t() = default;

    [[nodiscard]] explicit constexpr rect_fine_t(point_fine_t p0_, point_fine_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (p0_.x > p1_.x || p0_.y > p1_.y) [[unlikely]] {
            throw std::runtime_error("point in rect_t need to be ordered");
        }
    }

    [[nodiscard]] explicit constexpr rect_fine_t(point_t p0_, point_t p1_)
        : rect_fine_t {point_fine_t {p0_}, point_fine_t {p1_}} {};

    [[nodiscard]] explicit constexpr rect_fine_t(rect_t rect) noexcept
        : p0 {point_fine_t {rect.p0}}, p1 {point_fine_t {rect.p1}} {}

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const rect_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const rect_fine_t &other) const = default;

    [[nodiscard]] constexpr auto operator+(point_fine_t other) const -> rect_fine_t {
        return rect_fine_t {p0 + other, p1 + other};
    }

    [[nodiscard]] constexpr auto operator-(point_fine_t other) const -> rect_fine_t {
        return rect_fine_t {p0 - other, p1 - other};
    }

    constexpr auto operator+=(point_fine_t other) -> rect_fine_t & {
        p0 += other;
        p1 += other;
        return *this;
    }

    constexpr auto operator-=(point_fine_t other) -> rect_fine_t & {
        p0 -= other;
        p1 -= other;
        return *this;
    }
};

static_assert(std::is_trivial_v<rect_fine_t>);
static_assert(std::is_trivially_constructible_v<rect_fine_t>);
static_assert(std::is_trivially_copyable_v<rect_fine_t>);
static_assert(std::is_trivially_copy_assignable_v<rect_fine_t>);

}  // namespace logicsim

#endif
