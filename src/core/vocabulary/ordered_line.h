#ifndef LOGICSIM_VOCABULARY_ORDERED_LINE_H
#define LOGICSIM_VOCABULARY_ORDERED_LINE_H

#include "core/format/struct.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/point.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A horizontal or vertical line where the points are ordered.
 *
 * Class invariants:
 *     * points are ordered, p0 < p1
 *     * line [p0, p1] is either horizontal or vertical, not diagonal.
 *     * line has positive length, p0 != p1
 */
struct ordered_line_t {
    point_t p0 {};
    point_t p1 {};

    [[nodiscard]] explicit constexpr ordered_line_t() = default;
    [[nodiscard]] explicit constexpr ordered_line_t(point_t p0_, point_t p1_);
    [[nodiscard]] explicit constexpr ordered_line_t(line_t line) noexcept;

    [[nodiscard]] explicit constexpr operator line_t() const;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const ordered_line_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const ordered_line_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<ordered_line_t>);
static_assert(std::is_trivially_copy_assignable_v<ordered_line_t>);

//
// Implementation
//

constexpr ordered_line_t::ordered_line_t(point_t p0_, point_t p1_) : p0 {p0_}, p1 {p1_} {
    if (!(is_orthogonal_line(p0_, p1_) && p0_ < p1_)) [[unlikely]] {
        throw std::runtime_error(
            "line needs to be horizontal or vertical and points need to be ordered.");
    }
}

constexpr ordered_line_t::ordered_line_t(line_t line) noexcept
    : p0 {line.p0 < line.p1 ? line.p0 : line.p1},
      p1 {line.p0 < line.p1 ? line.p1 : line.p0} {}

constexpr ordered_line_t::operator line_t() const {
    return line_t {p0, p1};
}

}  // namespace logicsim

#endif
