#ifndef LOGICSIM_CORE_VOCABULARY_MOVE_DELTA_H
#define LOGICSIM_CORE_VOCABULARY_MOVE_DELTA_H

#include "core/format/struct.h"

#include <gsl/gsl>

namespace logicsim {

struct move_delta_t {
    int x {};
    int y {};

    [[nodiscard]] auto operator==(const move_delta_t &) const -> bool = default;
    [[nodiscard]] auto operator<=>(const move_delta_t &) const = default;
    [[nodiscard]] auto format() const -> std::string;

    // arithmetic (overflow is checked)
    //
    constexpr auto operator-() -> move_delta_t;
    constexpr auto operator+=(const move_delta_t &right) -> move_delta_t &;
};

static_assert(std::is_trivially_copyable_v<move_delta_t>);
static_assert(std::is_trivially_copy_assignable_v<move_delta_t>);
static_assert(std::regular<move_delta_t>);

[[nodiscard]] constexpr auto operator+(const move_delta_t &left,
                                       const move_delta_t &right) -> move_delta_t;

//
// Implementation
//

constexpr auto move_delta_t::operator-() -> move_delta_t {
    auto new_x = -int64_t {x};
    auto new_y = -int64_t {y};

    static_assert(sizeof(new_x) > sizeof(x));
    static_assert(sizeof(new_y) > sizeof(y));

    return move_delta_t {
        gsl::narrow<decltype(x)>(new_x),
        gsl::narrow<decltype(y)>(new_y),
    };
}

constexpr auto move_delta_t::operator+=(const move_delta_t &right) -> move_delta_t & {
    auto new_x = int64_t {x} + int64_t {right.x};
    auto new_y = int64_t {y} + int64_t {right.y};

    static_assert(sizeof(new_x) > sizeof(x));
    static_assert(sizeof(new_y) > sizeof(y));

    x = gsl::narrow<decltype(x)>(new_x);
    y = gsl::narrow<decltype(y)>(new_y);

    return *this;
}

constexpr auto operator+(const move_delta_t &left, const move_delta_t &right)
    -> move_delta_t {
    auto result = left;
    result += right;
    return result;
}

}  // namespace logicsim

#endif
