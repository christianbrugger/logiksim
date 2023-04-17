#ifndef LOGIKSIM_EDITABLE_CIRCUIT_SANITIZER_H
#define LOGIKSIM_EDITABLE_CIRCUIT_SANITIZER_H

namespace logicsim {

class CollisionCache;
class Selection;
class Layout;

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionCache &cache) -> void;

auto sanitize_selection_simple(Selection &selection, const Layout &layout,
                               const CollisionCache &cache) -> void;

}  // namespace logicsim

#endif