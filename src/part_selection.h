#ifndef LOGICSIM_PART_SELECTION_H
#define LOGICSIM_PART_SELECTION_H

#include "format/struct.h"
#include "vocabulary/part.h"

#include <folly/small_vector.h>

#include <compare>

namespace logicsim {

namespace part_selection {

// all parts are part of a line, so at maximum #grids / 2 selections are possible.
using part_vector_size_type = uint16_t;
constexpr inline auto part_vector_size = 2;

using part_vector_t = folly::small_vector<
    part_t, part_vector_size,
    folly::small_vector_policy::policy_size_type<part_vector_size_type>>;

static_assert(sizeof(part_vector_t) == 10);

}  // namespace part_selection

/**
* @brief: Selected parts on a line.
* 
* Class-invariants:
*     + parts_ are sorted ascending.
*     + adjacent parts do not touch (they are merged).
*/
class PartSelection {
   public:
    using part_vector_t = part_selection::part_vector_t;

   public:
    [[nodiscard]] auto format() const -> std ::string;

    [[nodiscard]] auto add_part(part_t part) -> void;
    [[nodiscard]] auto remove_part(part_t part) -> void;

    // [[nodiscard]] auto copy_selection(part_t) -> void;

    [[nodiscard]] auto operator==(const PartSelection&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PartSelection&) const = default;

   private:
    part_vector_t parts_;
};

}  // namespace logicsim

#endif
