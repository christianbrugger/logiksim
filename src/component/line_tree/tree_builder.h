#ifndef LOGICSIM_COMPONENT_LINE_TREE_TREE_BUILDER_H
#define LOGICSIM_COMPONENT_LINE_TREE_TREE_BUILDER_H

#include <span>

namespace logicsim {

struct ordered_line_t;
struct point_t;

namespace line_tree {

class LineStore;

/**
 * @brief: Fills the line store with the segments in depth first order.
 *
 * Pre-condition: segments are expected to form a contiguous tree.
 */
[[nodiscard]] auto create_line_store(std::span<const ordered_line_t> segments,
                                     point_t new_root) -> LineStore;

}  // namespace line_tree

}  // namespace logicsim

#endif
