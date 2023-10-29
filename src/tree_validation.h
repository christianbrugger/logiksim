#ifndef LOGICSIM_TREE_VALIDATION_H
#define LOGICSIM_TREE_VALIDATION_H

#include <span>

namespace logicsim {

struct ordered_line_t;

/**
 * @brief: Check if segments form a normalized tree.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting,
 * or don't form a loop free, connected tree.
 */
[[nodiscard]] auto segments_are_normalized_tree(std::vector<ordered_line_t> &&segments)
    -> bool;

}  // namespace logicsim

#endif
