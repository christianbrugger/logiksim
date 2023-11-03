#ifndef LOGICSIM_TREE_VALIDATION_H
#define LOGICSIM_TREE_VALIDATION_H

#include <vector>

namespace logicsim {

struct ordered_line_t;
class SegmentTree;

/**
 * @brief: Check if segments form a normalized tree.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting,
 * or don't form a loop free, connected tree.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto segments_are_contiguous_tree(std::vector<ordered_line_t>&& segments)
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

}  // namespace logicsim

#endif
