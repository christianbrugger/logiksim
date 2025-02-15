#ifndef LOGICSIM_VOCABULARY_LINE_H
#define LOGICSIM_VOCABULARY_LINE_H

#include "core/format/struct.h"
#include "core/vocabulary/point.h"

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A discrete horizontal or vertical line.
 *
 * Class invariants:
 *     * line [p0, p1] is either horizontal or vertical, not diagonal.
 *     * line has positive length, p0 != p1
 */
struct line_t {
    point_t p0 {};
    point_t p1 {};

    [[nodiscard]] explicit constexpr line_t() = default;
    [[nodiscard]] explicit constexpr line_t(point_t p0_, point_t p1_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const line_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const line_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<line_t>);
static_assert(std::is_trivially_copy_assignable_v<line_t>);

//
// Implementation
//

constexpr line_t::line_t(point_t p0_, point_t p1_) : p0 {p0_}, p1 {p1_} {
    if (!is_orthogonal_line(p0_, p1_)) [[unlikely]] {
        throw std::runtime_error("line needs to be horizontal or vertical.");
    }
};

}  // namespace logicsim

#endif
