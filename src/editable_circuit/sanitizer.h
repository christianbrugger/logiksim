#ifndef LOGIKSIM_EDITABLE_CIRCUIT_SANITIZER_H
#define LOGIKSIM_EDITABLE_CIRCUIT_SANITIZER_H

#include "format.h"

namespace logicsim {

class CollisionCache;
class Selection;
class Layout;
struct segment_part_t;

enum class SanitizeMode {
    expand,
    shrink,
};

template <>
auto format(SanitizeMode mode) -> std::string;

auto sanitize_part(segment_part_t segment_part, const Layout &layout,
                   const CollisionCache &cache, SanitizeMode mode) -> segment_part_t;

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionCache &cache, SanitizeMode mode) -> void;

}  // namespace logicsim

#endif