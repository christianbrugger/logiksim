
#include "editable_circuit/caches.h"

namespace logicsim {

//
// CacheProvider
//

auto CacheProvider::add_circuit(const Circuit& circuit) -> void {
    *this = CacheProvider {};

    // TODO consider bulk insertion, especially for spatial_cache_
    // add_circuit_to_cache(input_connections_, circuit);
    // add_circuit_to_cache(output_connections_, circuit);
    // add_circuit_to_cache(collision_cache_, circuit);
    // add_circuit_to_cache(spatial_cache_, circuit);
}

auto CacheProvider::format() const -> std::string {
    return fmt::format(
        "EditableCircuit::CacheProvider{{\n"
        "{}\n{}\n{}\n{}\n"
        "}}\n",
        input_connections_, output_connections_, collision_cache_, spatial_cache_);
}

auto CacheProvider::validate(const Circuit& circuit) -> void {
    spatial_cache_.validate(circuit);
    collision_cache_.validate(circuit);
    input_connections_.validate(circuit);
    output_connections_.validate(circuit);
}

auto CacheProvider::query_selection(rect_fine_t rect) const
    -> std::vector<SpatialTree::query_result_t> {
    return spatial_cache_.query_selection(rect);
};

auto CacheProvider::query_selection(point_fine_t point) const
    -> std::optional<element_id_t> {
    auto query_result = spatial_cache_.query_selection(rect_fine_t {point, point});

    // TODO rethink this, maybe use find?
    auto elements = std::vector<element_id_t> {};
    transform_if(
        query_result, std::back_inserter(elements),
        [](const SpatialTree::query_result_t& value) { return value.element_id; },
        [](const SpatialTree::query_result_t& value) {
            return value.segment_index == null_segment_index;
        });

    if (elements.size() > 1) [[unlikely]] {
        throw_exception("Two elements at the same position");
    }

    if (elements.size() == 1) {
        return elements[0];
    }
    return std::nullopt;
}

auto CacheProvider::submit(editable_circuit::InfoMessage message) -> void {
    input_connections_.submit(message);
    output_connections_.submit(message);
    collision_cache_.submit(message);
    spatial_cache_.submit(message);
}

auto CacheProvider::input_cache() const -> const ConnectionCache<true>& {
    return input_connections_;
}

auto CacheProvider::output_cache() const -> const ConnectionCache<false>& {
    return output_connections_;
}

auto CacheProvider::collision_cache() const -> const CollisionCache& {
    return collision_cache_;
}

auto CacheProvider::spatial_cache() const -> const SpatialTree& {
    return spatial_cache_;
}

auto CacheProvider::is_element_colliding(layout_calculation_data_t data) const -> bool {
    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("Not supported for wires.");
    }

    return collision_cache_.is_colliding(data) || input_connections_.is_colliding(data)
           || output_connections_.is_colliding(data);
}

/*
auto cache_insert(element_id_t element_id) -> void {
    const auto data = to_layout_calculation_data(schematic_, layout_, element_id);

    // TODO should we add support for wires?
    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("Not supported for wires.");
    }

    input_connections_.insert(element_id, data);
    output_connections_.insert(element_id, data);
    collision_cache_.insert(element_id, data);
    spatial_cache_.insert(element_id, data);
}

auto cache_remove(element_id_t element_id) -> void {
    const auto data = to_layout_calculation_data(schematic_, layout_, element_id);

    // TODO should we add support for wires?
    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("Not supported for wires.");
    }

    input_connections_.remove(element_id, data);
    output_connections_.remove(element_id, data);
    collision_cache_.remove(element_id, data);
    spatial_cache_.remove(element_id, data);
}

auto cache_update(element_id_t new_element_id, element_id_t old_element_id) -> void {
    if (schematic_.element(new_element_id).is_placeholder()) {
        return;
    }
    if (!is_element_cached(new_element_id)) {
        return;
    }

    // element cache update
    const auto data = to_layout_calculation_data(schematic_, layout_, new_element_id);

    if (data.element_type != ElementType::wire) {
        // TODO here we need to support wires
        input_connections_.update(new_element_id, old_element_id, data);
        output_connections_.update(new_element_id, old_element_id, data);
    }
    collision_cache_.update(new_element_id, old_element_id, data);
    spatial_cache_.update(new_element_id, old_element_id, data);
}
*/

}  // namespace logicsim
