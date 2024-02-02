#include "component/editable_circuit/layout_index.h"

#include "timer.h"

#include <fmt/core.h>

namespace logicsim {

//
// LayoutIndex
//

LayoutIndex::LayoutIndex(const Layout& layout) {
    const auto _ = Timer {"Layout Index"};

    // TODO consider bulk insertion, especially for spatial_index_
    {
        const auto _1 = Timer {"  Connection Index"};

        logicitems_inputs_ = LogicItemInputIndex {layout};
        logicitems_outputs_ = LogicItemOutputIndex {layout};
        wire_inputs_ = WireInputIndex {layout};
        wire_outputs_ = WireOutputIndex {layout};
    }

    {
        const auto _2 = Timer {"  Collision Index"};
        collision_index_ = CollisionIndex {layout};
    }
    {
        const auto _3 = Timer {"  Spatial Index"};
        spatial_index_ = SpatialIndex {layout};
    }
}

auto LayoutIndex::format() const -> std::string {
    return fmt::format(
        "EditableCircuit::LayoutIndex{{\n"
        "{}\n{}\n{}\n{}\n{}\n{}\n"
        "}}\n",
        logicitems_inputs_, logicitems_outputs_, wire_inputs_, wire_outputs_,
        collision_index_, spatial_index_);
}

auto LayoutIndex::allocated_size() const -> std::size_t {
    return logicitems_inputs_.allocated_size() +   //
           logicitems_outputs_.allocated_size() +  //
           wire_inputs_.allocated_size() +         //
           wire_outputs_.allocated_size() +        //

           collision_index_.allocated_size() +  //
           spatial_index_.allocated_size();
}

auto LayoutIndex::validate(const Layout& layout) -> void {
    logicitems_inputs_.validate(layout);
    logicitems_outputs_.validate(layout);
    wire_inputs_.validate(layout);
    wire_outputs_.validate(layout);

    spatial_index_.validate(layout);
    collision_index_.validate(layout);
}

auto LayoutIndex::submit(const editable_circuit::InfoMessage& message) -> void {
    logicitems_inputs_.submit(message);
    logicitems_outputs_.submit(message);
    wire_inputs_.submit(message);
    wire_outputs_.submit(message);

    collision_index_.submit(message);
    spatial_index_.submit(message);
}

auto LayoutIndex::logicitem_input_index() const -> const LogicItemInputIndex& {
    return logicitems_inputs_;
}

auto LayoutIndex::logicitem_output_index() const -> const LogicItemOutputIndex& {
    return logicitems_outputs_;
}

auto LayoutIndex::wire_input_index() const -> const WireInputIndex& {
    return wire_inputs_;
}

auto LayoutIndex::wire_output_index() const -> const WireOutputIndex& {
    return wire_outputs_;
}

auto LayoutIndex::collision_index() const -> const CollisionIndex& {
    return collision_index_;
}

auto LayoutIndex::selection_index() const -> const SpatialIndex& {
    return spatial_index_;
}

}  // namespace logicsim
