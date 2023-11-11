#include "editable_circuit/cache.h"

#include "editable_circuit/cache/helper.h"

namespace logicsim {

//
// CacheProvider
//

CacheProvider::CacheProvider(const Layout& layout) {
    // TODO consider bulk insertion, especially for spatial_cache_
    add_layout_to_cache(logicitems_inputs_, layout);
    add_layout_to_cache(logicitems_outputs_, layout);
    add_layout_to_cache(wire_inputs_, layout);
    add_layout_to_cache(wire_outputs_, layout);

    add_layout_to_cache(collision_cache_, layout);
    add_layout_to_cache(spatial_cache_, layout);
}

auto CacheProvider::format() const -> std::string {
    return fmt::format(
        "EditableCircuit::CacheProvider{{\n"
        "{}\n{}\n{}\n{}\n{}\n{}\n"
        "}}\n",
        logicitems_inputs_, logicitems_outputs_, wire_inputs_, wire_outputs_,
        collision_cache_, spatial_cache_);
}

auto CacheProvider::allocated_size() const -> std::size_t {
    return logicitems_inputs_.allocated_size() +   //
           logicitems_outputs_.allocated_size() +  //
           wire_inputs_.allocated_size() +         //
           wire_outputs_.allocated_size() +        //

           collision_cache_.allocated_size() +  //
           spatial_cache_.allocated_size();
}

auto CacheProvider::validate(const Layout& layout) -> void {
    logicitems_inputs_.validate(layout);
    logicitems_outputs_.validate(layout);
    wire_inputs_.validate(layout);
    wire_outputs_.validate(layout);

    spatial_cache_.validate(layout);
    collision_cache_.validate(layout);
}

auto CacheProvider::submit(const editable_circuit::InfoMessage& message) -> void {
    logicitems_inputs_.submit(message);
    logicitems_outputs_.submit(message);
    wire_inputs_.submit(message);
    wire_outputs_.submit(message);

    collision_cache_.submit(message);
    spatial_cache_.submit(message);
}

auto CacheProvider::logicitem_input_cache() const -> const LogicItemInputCache& {
    return logicitems_inputs_;
}

auto CacheProvider::logicitem_output_cache() const -> const LogicItemOutputCache& {
    return logicitems_outputs_;
}

auto CacheProvider::wire_input_cache() const -> const WireInputCache& {
    return wire_inputs_;
}

auto CacheProvider::wire_output_cache() const -> const WireOutputCache& {
    return wire_outputs_;
}

auto CacheProvider::collision_cache() const -> const CollisionCache& {
    return collision_cache_;
}

auto CacheProvider::spatial_cache() const -> const SpatialTree& {
    return spatial_cache_;
}

}  // namespace logicsim
