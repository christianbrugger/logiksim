#ifndef LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_LAYOUT_INDEX_H
#define LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_LAYOUT_INDEX_H

#include "index/collision_index.h"
#include "index/connection_index.h"
#include "index/selection_index.h"
#include "format/struct.h"
#include "layout_message_forward.h"

namespace logicsim {

class LayoutIndex {
   public:
    LayoutIndex() = default;
    explicit LayoutIndex(const Layout& layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto operator==(const LayoutIndex&) const -> bool = default;

    [[nodiscard]] auto logicitem_input_cache() const -> const LogicItemInputIndex&;
    [[nodiscard]] auto logicitem_output_cache() const -> const LogicItemOutputIndex&;
    [[nodiscard]] auto wire_input_cache() const -> const WireInputIndex&;
    [[nodiscard]] auto wire_output_cache() const -> const WireOutputIndex&;

    [[nodiscard]] auto collision_index() const -> const CollisionIndex&;
    [[nodiscard]] auto spatial_cache() const -> const SelectionIndex&;

    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto validate(const Layout& layout) -> void;

   private:
    LogicItemInputIndex logicitems_inputs_ {};
    LogicItemOutputIndex logicitems_outputs_ {};
    WireInputIndex wire_inputs_ {};
    WireOutputIndex wire_outputs_ {};

    CollisionIndex collision_cache_ {};
    SelectionIndex spatial_cache_ {};
};

static_assert(std::regular<LayoutIndex>);

}  // namespace logicsim

#endif
