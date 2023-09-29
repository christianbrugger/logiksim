#ifndef LOGICSIM_VOCABULARY_RECT_H
#define LOGICSIM_VOCABULARY_RECT_H

#include "format/struct.h"
#include "vocabulary/point.h"

#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A discrete 2-d rect defined by two ordered points p0 and p1.
 */
struct rect_t {
    point_t p0;
    point_t p1;

    [[nodiscard]] explicit constexpr rect_t() = default;
    [[nodiscard]] explicit constexpr rect_t(point_t p0_, point_t p1_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const rect_t &other) const -> bool = default;
};

static_assert(std::is_trivial_v<rect_t>);
static_assert(std::is_trivially_constructible_v<rect_t>);
static_assert(std::is_trivially_copyable_v<rect_t>);
static_assert(std::is_trivially_copy_assignable_v<rect_t>);

//
// Implementation
//

constexpr rect_t::rect_t(point_t p0_, point_t p1_) : p0 {p0_}, p1 {p1_} {
    if (p0_.x > p1_.x || p0_.y > p1_.y) [[unlikely]] {
        throw std::runtime_error("point in rect_t need to be ordered");
    }
};

}  // namespace logicsim

#endif
