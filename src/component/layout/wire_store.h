#ifndef LOGICSIM_COMPONENT_LAYOUT_WIRE_STORE_H
#define LOGICSIM_COMPONENT_LAYOUT_WIRE_STORE_H

#include "line_tree.h"
#include "segment_tree.h"

#include <vector>

namespace logicsim {

struct wire_id_t;

namespace layout {

/**
 * @brief: Stores the wires of the layout.
 *
 * Note the first and second wire have special meaning and are always present.
 * They can be accessed with `temporary_wire_id` and `colliding_wire_id`.
 *
 * Class invariants:
 *     + segment_trees_ and line_trees_ have same size
 *     + invalid and temporary wires are always present
 *     + Bounding rect either stores `invalid_bounding_rect` or the correct bounding rect.
 */
class WireStore {
   public:
    [[nodiscard]] explicit WireStore();

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    /**
     * @brief: brings the store in canonical form,
     *         so that visual equivalent layouts compare equal
     */
    auto normalize() -> void;
    [[nodiscard]] auto operator==(const WireStore &) const -> bool;

    // add & delete
    auto add_wire() -> wire_id_t;
    auto swap_and_delete(wire_id_t wire_id) -> wire_id_t;
    auto swap(wire_id_t wire_id_1, wire_id_t wire_id_2) -> void;

    // getters
    [[nodiscard]] auto segment_tree(wire_id_t wire_id) const -> const SegmentTree &;
    /**
     * @brief: Converts the segment tree to a line tree.
     *
     * Returns std::nullopt, if the tree has loops, overlaps or is disjointed.
     */
    [[nodiscard]] auto line_tree(wire_id_t wire_id) const
        -> const std::optional<LineTree> &;
    [[nodiscard]] auto bounding_rect(wire_id_t wire_id) const -> rect_t;

    // modifiable access
    [[nodiscard]] auto modifiable_segment_tree(wire_id_t wire_id) -> SegmentTree &;

   private:
    auto delete_last() -> void;
    auto last_wire_id() -> wire_id_t;

   private:
    std::vector<SegmentTree> segment_trees_ {};

    mutable std::vector<bool> line_tree_outdated_ {};
    mutable std::vector<std::optional<LineTree>> line_trees_ {};
    mutable std::vector<rect_t> bounding_rects_ {};  // TODO where is rect_t coming from?
};

const auto sa = sizeof(std::optional<LineTree>);
const auto sb = sizeof(LineTree);

}  // namespace layout

}  // namespace logicsim

#endif
