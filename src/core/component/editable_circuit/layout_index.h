#ifndef LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_LAYOUT_INDEX_H
#define LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_LAYOUT_INDEX_H

#include "core/format/struct.h"
#include "core/index/collision_index.h"
#include "core/index/connection_index.h"
#include "core/index/key_index.h"
#include "core/index/spatial_index.h"
#include "core/layout_message_forward.h"

namespace logicsim {

/**
 * @brief: Efficiently stores connection, collision info and selections of the Layout.
 *
 * Pre-conditions:
 *   + inserted wire segments need to have the correct SegmentPointType
 *   + requires a correct history of messages of element changes
 *
 * Class-invariants:
 *   + There are no duplicate connections of the same type for inserted elements.
 *   + Inserted wires & logicitems are not colliding.
 *
 */
class LayoutIndex {
   public:
    [[nodiscard]] explicit LayoutIndex() = default;
    [[nodiscard]] explicit LayoutIndex(const Layout& layout);
    [[nodiscard]] explicit LayoutIndex(const Layout& layout, KeyIndex key_index);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto operator==(const LayoutIndex&) const -> bool = default;

    [[nodiscard]] auto logicitem_input_index() const -> const LogicItemInputIndex&;
    [[nodiscard]] auto logicitem_output_index() const -> const LogicItemOutputIndex&;
    [[nodiscard]] auto wire_input_index() const -> const WireInputIndex&;
    [[nodiscard]] auto wire_output_index() const -> const WireOutputIndex&;

    [[nodiscard]] auto collision_index() const -> const CollisionIndex&;
    [[nodiscard]] auto selection_index() const -> const SpatialIndex&;
    [[nodiscard]] auto key_index() const -> const KeyIndex&;

    auto submit(const InfoMessage& message) -> void;

   private:
    LogicItemInputIndex logicitems_inputs_ {};
    LogicItemOutputIndex logicitems_outputs_ {};
    WireInputIndex wire_inputs_ {};
    WireOutputIndex wire_outputs_ {};

    CollisionIndex collision_index_ {};
    SpatialIndex spatial_index_ {};
    KeyIndex key_index_ {};
};

static_assert(std::regular<LayoutIndex>);

}  // namespace logicsim

#endif
