#ifndef LOGICSIM_VOCABULARY_PART_H
#define LOGICSIM_VOCABULARY_PART_H

#include "format/struct.h"
#include "vocabulary/offset.h"

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Part defines a sub-segment of an ordered line from [p0 + begin, p1 + end].
 *
 * Class invariants:
 *     * begin and end are ordered & length is positive: begin < end
 */
struct part_t {
    offset_t begin;
    offset_t end;

    [[nodiscard]] explicit constexpr part_t(offset_like auto begin_,
                                            offset_like auto end_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const part_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const part_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<part_t>);
static_assert(std::is_trivially_copy_assignable_v<part_t>);

//
// Implementation
//

constexpr part_t::part_t(offset_like auto begin_, offset_like auto end_)
    : begin {offset_t {begin_}}, end {offset_t {end_}} {
    if (!(begin < end)) [[unlikely]] {
        throw std::runtime_error("begin needs to be smaller than end.");
    }
};

}  // namespace logicsim

#endif
