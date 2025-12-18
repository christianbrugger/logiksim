#include "core/component/editable_circuit/layout_index.h"

#include "core/vocabulary/allocation_info.h"

#include <fmt/core.h>

namespace logicsim {

//
// LayoutIndex
//

LayoutIndex::LayoutIndex(const Layout& layout)
    : LayoutIndex {layout, KeyIndex {layout}} {}

LayoutIndex::LayoutIndex(const Layout& layout, KeyIndex key_index)
    : logicitems_inputs_ {layout},
      logicitems_outputs_ {layout},
      wire_inputs_ {layout},
      wire_outputs_ {layout},

      collision_index_ {layout},
      spatial_index_ {layout},
      key_index_ {std::move(key_index)} {}

auto LayoutIndex::format() const -> std::string {
    return fmt::format(
        "EditableCircuit::LayoutIndex{{\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "{}\n"
        "}}\n",
        logicitems_inputs_, logicitems_outputs_, wire_inputs_, wire_outputs_,
        collision_index_, spatial_index_, key_index_);
}

auto LayoutIndex::allocated_size() const -> std::size_t {
    return logicitems_inputs_.allocated_size() +   //
           logicitems_outputs_.allocated_size() +  //
           wire_inputs_.allocated_size() +         //
           wire_outputs_.allocated_size() +        //

           collision_index_.allocated_size() +  //
           spatial_index_.allocated_size() +    //
           key_index_.allocated_size();
}

auto LayoutIndex::allocation_info() const -> LayoutIndexAllocInfo {
    return LayoutIndexAllocInfo {
        .connection_index = Byte {logicitems_inputs_.allocated_size() +   //
                                  logicitems_outputs_.allocated_size() +  //
                                  wire_inputs_.allocated_size() +         //
                                  wire_outputs_.allocated_size()},
        .collision_index = Byte {collision_index_.allocated_size()},
        .spatial_index = Byte {spatial_index_.allocated_size()},
        .key_index = Byte {key_index_.allocated_size()},
    };
}

auto LayoutIndex::submit(const InfoMessage& message) -> void {
    logicitems_inputs_.submit(message);
    logicitems_outputs_.submit(message);
    wire_inputs_.submit(message);
    wire_outputs_.submit(message);

    collision_index_.submit(message);
    spatial_index_.submit(message);
    key_index_.submit(message);
}

auto LayoutIndex::set_key(decoration_id_t decoration_id, decoration_key_t decoration_key)
    -> void {
    key_index_.set(decoration_id, decoration_key);
}

auto LayoutIndex::set_key(logicitem_id_t logicitem_id, logicitem_key_t logicitem_key)
    -> void {
    key_index_.set(logicitem_id, logicitem_key);
}

auto LayoutIndex::set_key(segment_t segment, segment_key_t segment_key) -> void {
    key_index_.set(segment, segment_key);
}

auto LayoutIndex::swap_key(segment_t segment_0, segment_t segment_1) -> void {
    key_index_.swap(segment_0, segment_1);
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

auto LayoutIndex::key_index() const -> const KeyIndex& {
    return key_index_;
}

}  // namespace logicsim
