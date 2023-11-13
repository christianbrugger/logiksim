#include "component/editable_circuit/layout_index.h"

#include "layout_message_generation.h"

#include <fmt/core.h>

namespace logicsim {

//
// LayoutIndex
//

LayoutIndex::LayoutIndex(const Layout& layout) {
    // TODO consider bulk insertion, especially for spatial_index_
    add_layout_to_cache(logicitems_inputs_, layout);
    add_layout_to_cache(logicitems_outputs_, layout);
    add_layout_to_cache(wire_inputs_, layout);
    add_layout_to_cache(wire_outputs_, layout);

    add_layout_to_cache(collision_index_, layout);
    add_layout_to_cache(spatial_index_, layout);
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

auto LayoutIndex::selection_index() const -> const SelectionIndex& {
    return spatial_index_;
}

}  // namespace logicsim
