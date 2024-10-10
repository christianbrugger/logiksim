#ifndef LOGICSIM_CORE_VOCABULARY_SIZE_H
#define LOGICSIM_CORE_VOCABULARY_SIZE_H

#include "format/struct.h"
#include "vocabulary/offset.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A discrete size in 2 dimensions.
 *
 * We avoid size_t due to conflicts with C headers.
 */
struct size_2d_t {
    offset_t width {};
    offset_t height {};

    [[nodiscard]] explicit constexpr size_2d_t() = default;
    [[nodiscard]] explicit constexpr size_2d_t(offset_like auto width_,
                                               offset_like auto height_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const size_2d_t &other) const -> bool =
                                                                                 default;
    [[nodiscard]] constexpr auto operator<=>(const size_2d_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<size_2d_t>);
static_assert(std::is_trivially_copy_assignable_v<size_2d_t>);

//
// Implementation
//

constexpr size_2d_t::size_2d_t(offset_like auto width_, offset_like auto height_)
    : width {offset_t {width_}}, height {offset_t {height_}} {};

}  // namespace logicsim

#endif
