#ifndef LOGICSIM_TREE_NORMALIZATION_H
#define LOGICSIM_TREE_NORMALIZATION_H

#include <span>
#include <vector>

namespace logicsim {

struct ordered_line_t;
class SegmentTree;

/**
 * @brief: Splits and merges overlapping segments.
 *
 * First Overlapping or connecting & parallel segments are merged.
 * Then all lines colliding with cross-points are split.
 *
 * The algorithm is O(N log N).
 *
 * Returns a flat list of segments, where there are no internal colliding points.
 */
[[nodiscard]] auto merge_split_segments(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t>;

/**
 * @brief: Check if segments form are normalized.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto segments_are_normalized(std::span<const ordered_line_t> segments)
    -> bool;

/**
 * @brief: Check if segments form a normalized tree.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting,
 * or don't form a loop free, connected tree. Or the tree is empty.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto segments_are_contiguous_tree(std::span<const ordered_line_t> segments)
    -> bool;

/**
 * @brief: Check if segment tree is a contiguous tree.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting,
 * or don't form a loop free, connected tree.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto is_contiguous_tree(const SegmentTree& tree) -> bool;

/**
 * @brief: Check if segment tree has correctly set endpoints.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto has_correct_endpoints(const SegmentTree& tree) -> bool;

/**
 * @brief: Check if segment tree is a contiguous tree with correctly set endpoints.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting,
 * or don't form a loop free, connected tree, or endpoints are not correct.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto is_contiguous_tree_with_correct_endpoints(const SegmentTree& tree)
    -> bool;

}  // namespace logicsim

#endif
