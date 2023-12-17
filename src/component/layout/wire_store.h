#ifndef LOGICSIM_COMPONENT_LAYOUT_WIRE_STORE_H
#define LOGICSIM_COMPONENT_LAYOUT_WIRE_STORE_H

#include "segment_tree.h"
#include "vocabulary/rect.h"

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
 * Note segments in the segment tree can be any set of valid line segments,
 * only subject to the class-invariants of SegmentTree.
 *
 * Class invariants:
 *     + segment_trees_ and bounding_rects_ have same size
 *     + invalid and temporary wires are always present
 *     + bounding_rects_ either stores `invalid_bounding_rect` or the correct rect.
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
    [[nodiscard]] auto bounding_rect(wire_id_t wire_id) const -> rect_t;

    // modifiable access
    [[nodiscard]] auto modifiable_segment_tree(wire_id_t wire_id) -> SegmentTree &;

   private:
    auto delete_last() -> void;
    auto last_wire_id() -> wire_id_t;

   private:
    std::vector<SegmentTree> segment_trees_ {};
    mutable std::vector<rect_t> bounding_rects_ {};
};

}  // namespace layout

}  // namespace logicsim

#endif
