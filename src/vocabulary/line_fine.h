#ifndef LOGICSIM_VOCABULARY_LINE_FINE_H
#define LOGICSIM_VOCABULARY_LINE_FINE_H

#include "format/struct.h"
#include "vocabulary/line.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point_fine.h"

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A continuous horizontal or vertical line.
 */
struct line_fine_t {
    point_fine_t p0;
    point_fine_t p1;

    [[nodiscard]] explicit constexpr line_fine_t() = default;

    [[nodiscard]] explicit constexpr line_fine_t(line_t line) noexcept;
    [[nodiscard]] explicit constexpr line_fine_t(ordered_line_t line) noexcept;

    [[nodiscard]] explicit constexpr line_fine_t(point_fine_like auto p0_,
                                                 point_fine_like auto p1_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const line_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const line_fine_t &other) const = default;
};

static_assert(std::is_trivial_v<line_fine_t>);
static_assert(std::is_trivially_constructible_v<line_fine_t>);
static_assert(std::is_trivially_copyable_v<line_fine_t>);
static_assert(std::is_trivially_copy_assignable_v<line_fine_t>);

//
// Implementation
//

constexpr line_fine_t::line_fine_t(line_t line) noexcept
    : p0 {point_fine_t {line.p0}}, p1 {point_fine_t {line.p1}} {}

constexpr line_fine_t::line_fine_t(ordered_line_t line) noexcept
    : p0 {point_fine_t {line.p0}}, p1 {point_fine_t {line.p1}} {}

constexpr line_fine_t::line_fine_t(point_fine_like auto p0_, point_fine_like auto p1_)
    : p0 {point_fine_t {p0_}}, p1 {point_fine_t {p1_}} {
    if (!is_orthogonal_line(p0, p1)) [[unlikely]] {
        throw std::runtime_error("line needs to be horizontal or vertical.");
    }
};

}  // namespace logicsim

#endif
