#ifndef LOGICSIM_SELECTION_NORMALIZATION_H
#define LOGICSIM_SELECTION_NORMALIZATION_H

#include "format/enum.h"

namespace logicsim {

class CollisionIndex;
class Selection;
class Layout;
struct segment_part_t;

enum class SanitizeMode {
    expand,
    shrink,
};

template <>
[[nodiscard]] auto format(SanitizeMode mode) -> std::string;

auto sanitize_part(segment_part_t segment_part, const Layout &layout,
                   const CollisionIndex &cache, SanitizeMode mode) -> segment_part_t;

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionIndex &cache, SanitizeMode mode) -> void;

}  // namespace logicsim

#endif