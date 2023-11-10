#ifndef LOGICSIM_LINE_TREE_GENERATION_H
#define LOGICSIM_LINE_TREE_GENERATION_H

#include <optional>
#include <vector>

namespace logicsim {

class SegmentTree;
class LineTree;
class Layout;

[[nodiscard]] auto generate_line_tree(const SegmentTree& segment_tree)
    -> std::optional<LineTree>;
[[nodiscard]] auto generate_line_trees(const Layout& layout)
    -> std::vector<std::optional<LineTree>>;

[[nodiscard]] auto has_same_segments(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto has_same_cross_points(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto has_same_input_position(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto has_same_output_positions(const SegmentTree&, const LineTree&) -> bool;
[[nodiscard]] auto is_equivalent(const SegmentTree&, const LineTree&) -> bool;

}  // namespace logicsim

#endif
