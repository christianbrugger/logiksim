#ifndef LOGIKSIM_EDITABLE_CIRCUIT_SANITIZER_H
#define LOGIKSIM_EDITABLE_CIRCUIT_SANITIZER_H

#include "format.h"

namespace logicsim {

class CollisionCache;
class Selection;
class Layout;

enum class SanitizeMode {
    expand,
    shrink,
};

template <>
auto format(SanitizeMode mode) -> std::string;

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionCache &cache, SanitizeMode mode) -> void;

auto sanitize_selection_simple(Selection &selection, const Layout &layout,
                               const CollisionCache &cache) -> void;

}  // namespace logicsim

#endif