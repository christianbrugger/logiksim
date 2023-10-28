#ifndef LOGICSIM_COMPONENT_LINE_TREE_TREE_BUILDER_H
#define LOGICSIM_COMPONENT_LINE_TREE_TREE_BUILDER_H

#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"

#include <span>

namespace logicsim {

namespace line_tree {

class LineStore;

//[[nodiscard]] auto create_line_store_generic(std::span<const ordered_line_t> segments,
//                                             std::optional<point_t> new_root)
//    -> std::optional<LineStore>;

[[nodiscard]] auto create_line_store_simplified(std::span<const ordered_line_t> segments,
                                                point_t new_root) -> LineStore;

}  // namespace line_tree

}  // namespace logicsim

#endif
