#ifndef LOGICSIM_COMPONENT_LINE_TREE_LINE_STORE_H
#define LOGICSIM_COMPONENT_LINE_TREE_LINE_STORE_H

#include "core/format/struct.h"
#include "core/vocabulary/length.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/line_index.h"

#include <folly/small_vector.h>

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
 * Note that the first line is always a 'leaf' by construction.
 *
 * Class invariants:
 *     + lines_ and start_lengths_ have the same size
 *     + start_lengths_ contains the length from root to p0 of corresponding line.
 *     + leaf_indices_ point to all leaves.
 *     + lines_ are ordered in depth first order.
 *     + the points p1 of all lines are unique.
 */
class LineStore {
   public:
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

    [[nodiscard]] auto allocated_size() const -> std::size_t;
    auto reserve(std::size_t capacity) -> void;
    auto shrink_to_fit() -> void;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const LineStore &) const -> bool = default;

    /**
     * @brief: Adds the first line to the LineStore.
     *
     * Throws if the LineStore is not empty.
     */
    auto add_first_line(line_t new_line) -> line_index_t;

    /**
     * @brief: Adds a new line to the LineStore.
     *
     * Note lines must be added in depth first order.
     *
     * Throws if the line store is empty.
     * Throws if new line doesn't connect to old line.
     * Throws if previous index refers to a leaf and is not the last index.
     * Throws if added line point p1 is already part of the tree.
     *
     * Returns the index of the new line
     */
    auto add_line(line_t new_line, line_index_t previous_index) -> line_index_t;

    [[nodiscard]] auto line(line_index_t index) const -> line_t;
    [[nodiscard]] auto start_length(line_index_t index) const -> length_t;
    [[nodiscard]] auto end_length(line_index_t index) const -> length_t;
    [[nodiscard]] auto starts_new_subtree(line_index_t index) const -> bool;

    [[nodiscard]] auto lines() const -> const line_vector_t &;
    [[nodiscard]] auto start_lengths() const -> const length_vector_t &;
    [[nodiscard]] auto leaf_indices() const -> const index_vector_t &;

    [[nodiscard]] auto last_index() const -> line_index_t;

   private:
    // contain lines in depth-first order
    line_vector_t lines_ {};

    // contain start length of each line
    length_vector_t start_lengths_ {};

    // contain indices of leaf nodes
    index_vector_t leaf_indices_ {};
};

static_assert(sizeof(LineStore) == 36);

}  // namespace line_tree

}  // namespace logicsim

#endif
