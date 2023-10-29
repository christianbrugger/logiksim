#ifndef LOGICSIM_COMPONENT_LINE_TREE_TREE_BUILDER_H
#define LOGICSIM_COMPONENT_LINE_TREE_TREE_BUILDER_H

#include <span>

namespace logicsim {

struct ordered_line_t;
struct point_t;

namespace line_tree {

class LineStore;

[[nodiscard]] auto create_line_store(std::span<const ordered_line_t> segments,
                                     point_t new_root) -> LineStore;

}  // namespace line_tree

}  // namespace logicsim

#endif
