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
 * Pre-condition: segment-tree is expected to form a contiguous tree.
 * Pre-condition: segment-tree has correct cross-points set.
 */
[[nodiscard]] auto generate_line_tree(const SegmentTree& segment_tree) -> LineTree;

/**
 * @brief: Creates a vector with LineTrees for all wires.
 *
 * Pre-condition: All inserted segment-trees are expected to form contiguous trees.
 * Pre-condition: All inserted segment-trees have all cross-points set.
 *
 * Note the result contains empty LineTrees for all non-inserted wires.
 */
[[nodiscard]] auto generate_line_trees(const Layout& layout) -> std::vector<LineTree>;

[[nodiscard]] auto has_same_segments(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto has_same_cross_points(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto has_same_input_position(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto has_same_output_positions(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto is_equivalent(const SegmentTree&, const LineTree&) -> bool;

[[nodiscard]] auto all_wires_equivalent(const Layout& layout,
                                        const std::vector<LineTree>& line_trees) -> bool;

}  // namespace logicsim

#endif
