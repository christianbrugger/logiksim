#ifndef LOGICSIM_LINE_TREE_GENERATION_H
#define LOGICSIM_LINE_TREE_GENERATION_H

#include <vector>

namespace logicsim {

class SegmentTree;
class LineTree;
class Layout;

/**
 * @brief: Creates a LineTree from a SegmentTree.
 *
 * Pre-condition: segment-tree is contiguous tree with correct endpoints.
 *                See `tree_normalization.h`.
 */
[[nodiscard]] auto generate_line_tree(const SegmentTree& segment_tree) -> LineTree;

/**
 * @brief: Creates a vector with LineTrees for all wires.
 *
 * Pre-condition: segment-trees are contiguous tree with correct endpoints.
 *                See `tree_normalization.h`.
 *
 * Result contains LineTrees of inserted and empty trees for non-inserted wires.
 */
[[nodiscard]] auto generate_line_trees(const Layout& layout) -> std::vector<LineTree>;

[[nodiscard]] auto has_same_segments(const SegmentTree& segment_tree,
                                     const LineTree& line_tree) -> bool;
[[nodiscard]] auto has_same_cross_points(const SegmentTree& segment_tree,
                                         const LineTree& line_tree) -> bool;
[[nodiscard]] auto has_same_corner_points(const SegmentTree& segment_tree,
                                          const LineTree& line_tree) -> bool;
[[nodiscard]] auto has_same_input_position(const SegmentTree& segment_tree,
                                           const LineTree& line_tree) -> bool;
[[nodiscard]] auto has_same_output_positions(const SegmentTree& segment_tree,
                                             const LineTree& line_tree) -> bool;

/**
 * @brief: Checks if a segment-tree and line-tree are equivalent.
 *
 * Note this includes same segments, cross-points, corners, input and output positions.
 */
[[nodiscard]] auto is_equivalent(const SegmentTree& segment_tree,
                                 const LineTree& line_tree) -> bool;

/**
 * @brief: Checks if all lines-trees are equivalent to the segment-trees of the layout.
 */
[[nodiscard]] auto all_wires_equivalent(const Layout& layout,
                                        const std::vector<LineTree>& line_trees) -> bool;

}  // namespace logicsim

#endif
