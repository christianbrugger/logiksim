#ifndef LOGICSIM_COMPONENT_LINE_TREE_LINE_STORE_H
#define LOGICSIM_COMPONENT_LINE_TREE_LINE_STORE_H

#include "vocabulary/grid.h"
#include "vocabulary/length.h"
#include "vocabulary/line.h"
#include "vocabulary/line_index.h"
#include "vocabulary/point.h"

#include <folly/small_vector.h>

#include <optional>

namespace logicsim {

namespace line_tree {

using line_tree_vector_policy =
    folly::small_vector_policy::policy_size_type<line_index_t::value_type>;

using line_vector_t = folly::small_vector<line_t, 1, line_tree_vector_policy>;
using index_vector_t = folly::small_vector<line_index_t, 2, line_tree_vector_policy>;
using length_vector_t = folly::small_vector<length_t, 2, line_tree_vector_policy>;

static_assert(sizeof(line_vector_t) == 12);
static_assert(sizeof(index_vector_t) == 12);
static_assert(sizeof(length_vector_t) == 12);

/**
 * @ Stores the line of a tree in depth-first order.
 *
 * Pre-condition lines added do not collide with themselves.
 *
 * Note that the first line is always a leaf by construction.
 *
 * Class invariants:
 *     + lines_ and start_lengths_ have the same size
 *     + start_lengths_ contains the length from root to p0 of that line.
 *     + leaf_lines_ point to all leaves.
 *     + lines_ are added in depth first order.
 */
class LineStore {
   public:
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> std::size_t;

    auto reserve(std::size_t capacity) -> void;
    auto shrink_to_fit() -> void;

    /**
     * @brief: Adds the first line to the LineStore.
     *
     * Throws an exception if the LineStore is not empty.
     */
    auto add_first_line(line_t new_line) -> line_index_t;

    /**
     * @brief: Adds a new line to the LineStore.
     *
     * Pre-condition line added does collide with previous line.
     * (checked only in debug mode)
     *
     * Note lines must be added in depth first order.
     *
     * Throws if the line store is empty.
     * Throws if new line doesn't connect to old line.
     * Throws if previous index refers to a leaf and is not the last index.
     * Throws if previous index is the last index and orientation is the same.
     *
     * Returns the index of the new line
     */
    auto add_line(line_t new_line, line_index_t previous_line) -> line_index_t;

    [[nodiscard]] auto line(line_index_t index) const -> line_t;
    [[nodiscard]] auto start_length(line_index_t index) const -> length_t;
    [[nodiscard]] auto end_length(line_index_t index) const -> length_t;
    [[nodiscard]] auto starts_new_subtree(line_index_t index) const -> bool;

   private:
    [[nodiscard]] auto last_index() const -> line_index_t;

   private:
    line_vector_t lines_ {};
    length_vector_t start_lengths_ {};
    index_vector_t leaf_lines_ {};
};

static_assert(sizeof(LineStore) == 36);

}  // namespace line_tree

}  // namespace logicsim

#endif
