
#include "editable_circuit/caches.h"

#include "circuit.h"
#include "layout_calculations.h"

#include <type_traits>
#include <variant>

namespace logicsim {

//
// ConnectionCache
//

namespace detail::connection_cache {
inline auto value_type::format() const -> std::string {
    return fmt::format("<Element {}-{} {}>", connection_id, element_id, orientation);
}
}  // namespace detail::connection_cache

auto get_and_verify_cache_entry(detail::connection_cache::map_type& map, point_t position,
                                detail::connection_cache::value_type value)
    -> detail::connection_cache::map_type::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw_exception("unable to find chached data that should be present.");
    }
    return it;
}

template <bool IsInput>
auto ConnectionCache<IsInput>::format() const -> std::string {
    if constexpr (IsInput) {
        return fmt::format("InputCache = {}\n", connections_);
    } else {
        return fmt::format("OutputCache = {}\n", connections_);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::submit(editable_circuit::InfoMessage message) -> void {
    using namespace editable_circuit::info_message;

    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        return handle(*pointer);
    }
    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        return handle(*pointer);
    }
    if (auto pointer = std::get_if<InsertedLogicItemUpdated>(&message)) {
        return handle(*pointer);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::LogicItemInserted message) -> void {
    // placeholders are not cached
    if (is_placeholder(message.data)) {
        return;
    }

    const auto add_position = [&](connection_id_t connection_id, point_t position,
                                  orientation_t orientation) {
        if (connections_.contains(position)) [[unlikely]] {
            throw_exception("cache already has an entry at this position");
        }
        connections_[position] = {message.element_id, connection_id, orientation};
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, add_position);
    } else {
        iter_output_location_and_id(message.data, add_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::LogicItemUninserted message) -> void {
    // placeholders are not cached
    if (is_placeholder(message.data)) {
        return;
    }

    const auto remove_position = [&](connection_id_t connection_id, point_t position,
                                     orientation_t orientation) {
        const auto value = value_type {message.element_id, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(connections_, position, value);
        connections_.erase(it);
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, remove_position);
    } else {
        iter_output_location_and_id(message.data, remove_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::InsertedLogicItemUpdated message) -> void {
    // placeholders are not cached
    if (is_placeholder(message.data)) {
        return;
    }

    const auto update_id = [&](connection_id_t connection_id, point_t position,
                               orientation_t orientation) {
        const auto old_value
            = value_type {message.old_element_id, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(connections_, position, old_value);
        it->second.element_id = message.new_element_id;
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(message.data, update_id);
    } else {
        iter_output_location_and_id(message.data, update_id);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::SegmentInserted message) -> void {}

template <bool IsInput>
auto ConnectionCache<IsInput>::handle(
    editable_circuit::info_message::SegmentUninserted message) -> void {}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position) const
    -> std::optional<std::pair<connection_t, orientation_t>> {
    if (const auto it = connections_.find(position); it != connections_.end()) {
        return std::make_pair(
            connection_t {it->second.element_id, it->second.connection_id},
            it->second.orientation);
    }
    return std::nullopt;
}

template <bool IsInput>
auto to_connection_entry(auto&& schematic,
                         std::optional<std::pair<connection_t, orientation_t>> entry) {
    return std::make_optional(
        std::make_pair(to_connection<IsInput>(schematic, entry->first), entry->second));
}

template <bool IsInput>
auto find_impl(const ConnectionCache<IsInput>& cache, point_t position,
               auto&& schematic) {
    if (auto entry = cache.find(position)) {
        return to_connection_entry<IsInput>(schematic, entry);
    }
    // nullopt with correct type
    return decltype(to_connection_entry<IsInput>(schematic, {})) {};
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position, Schematic& schematic) const
    -> std::optional<std::pair<connection_proxy, orientation_t>> {
    return find_impl(*this, position, schematic);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::find(point_t position, const Schematic& schematic) const
    -> std::optional<std::pair<const_connection_proxy, orientation_t>> {
    return find_impl(*this, position, schematic);
}

template <bool IsInput>
auto ConnectionCache<IsInput>::is_colliding(layout_calculation_data_t data) const
    -> bool {
    // make sure inputs/outputs don't collide with inputs/outputs
    const auto same_type_not_colliding
        = [&](point_t position, orientation_t _ [[maybe_unused]]) -> bool {
        return !connections_.contains(position);
    };

    // make sure inputs match with output orientations, if present
    const auto different_type_compatible
        = [&](point_t position, orientation_t orientation) -> bool {
        if (const auto it = connections_.find(position); it != connections_.end()) {
            return orientations_compatible(orientation, it->second.orientation);
        }
        return true;
    };

    if constexpr (IsInput) {
        return !(iter_input_location(data, same_type_not_colliding)
                 && iter_output_location(data, different_type_compatible));
    } else {
        return !(iter_output_location(data, same_type_not_colliding)
                 && iter_input_location(data, different_type_compatible));
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::validate(const Circuit& circuit) const -> void {
    auto cache = ConnectionCache<IsInput> {};
    add_circuit_to_cache(cache, circuit);

    if (cache.connections_ != this->connections_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

template class ConnectionCache<true>;
template class ConnectionCache<false>;

//
// Collision Cache
//

namespace {
[[nodiscard]] auto has_vertical_element(CollisionCache::collision_data_t data) -> bool {
    return data.element_id_vertical >= element_id_t {0};
}

}  // namespace

// next_state(point_t position, ItemType state) -> bool
template <typename Func>
auto iter_body_collision_state(layout_calculation_data_t data, Func next_state) -> bool {
    if (!iter_input_location(
            data, [=](point_t position, orientation_t _ [[maybe_unused]]) {
                return next_state(position, CollisionCache::ItemType::element_connection);
            })) {
        return false;
    }

    if (!iter_element_body_points(data, [=](point_t position) {
            return next_state(position, CollisionCache::ItemType::element_body);
        })) {
        return false;
    }

    if (!iter_output_location(
            data, [=](point_t position, orientation_t _ [[maybe_unused]]) {
                return next_state(position, CollisionCache::ItemType::element_connection);
            })) {
        return false;
    }

    return true;
}

// next_state(point_t position, ItemType state) -> bool
template <typename Func>
auto iter_collision_state(layout_calculation_data_t data, Func next_state) -> bool {
    if (data.element_type == ElementType::placeholder) {
        return true;
    }
    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("not supported");
    }
    return iter_body_collision_state(data, next_state);
}

// next_state(point_t position, ItemType state) -> bool
template <typename Func>
auto iter_collision_state(segment_info_t segment, Func next_state) -> bool {
    using enum CollisionCache::ItemType;

    {
        const auto line = order_points(segment.line);

        if (is_horizontal(line)) {
            for (auto x : range(line.p0.x + grid_t {1}, line.p1.x)) {
                if (!next_state(point_t {x, line.p0.y}, wire_horizontal)) {
                    return false;
                }
            }
        } else {
            for (auto y : range(line.p0.y + grid_t {1}, line.p1.y)) {
                if (!next_state(point_t {line.p0.x, y}, wire_vertical)) {
                    return false;
                }
            }
        }
    }

    const auto to_state
        = [](SegmentPointType type) -> std::optional<CollisionCache::ItemType> {
        switch (type) {
            using enum SegmentPointType;

            case input:
            case output:
                return wire_connection;
            case colliding_point:
                return wire_point;
            case cross_point_horizontal:
                return wire_horizontal;
            case cross_point_vertical:
                return wire_vertical;

            case shadow_point:
            case visual_cross_point:
                return std::nullopt;

            case new_unknown:
                return wire_new_unknown_point;
        }
        throw_exception("unknown point type");
    };

    const auto p0_state = to_state(segment.p0_type);
    const auto p1_state = to_state(segment.p1_type);

    if (p0_state) {
        if (!next_state(segment.line.p0, *p0_state)) {
            return false;
        }
    }
    if (p1_state) {
        if (!next_state(segment.line.p1, *p1_state)) {
            return false;
        }
    }

    return true;
}

namespace {
// apply_func(element_id_t& obj) -> void
template <typename Apply>
auto apply_function(CollisionCache::map_type& map, point_t position,
                    CollisionCache::ItemType state, Apply apply_func) -> bool {
    auto& data = map[position];

    const auto set_connection_tag = [&]() {
        if (has_vertical_element(data)) {
            throw_exception("cannot set connection tag, second element occupied");
        }
        data.element_id_vertical = CollisionCache::connection_tag;
    };

    switch (state) {
        using enum CollisionCache::ItemType;

        case element_body: {
            apply_func(data.element_id_body);
            break;
        }
        case element_connection: {
            set_connection_tag();
            apply_func(data.element_id_body);
            break;
        }
        case wire_connection: {
            set_connection_tag();
            apply_func(data.element_id_horizontal);
            break;
        }
        case wire_horizontal: {
            apply_func(data.element_id_horizontal);
            break;
        }
        case wire_vertical: {
            apply_func(data.element_id_vertical);
            break;
        }
        case wire_point: {
            apply_func(data.element_id_horizontal);
            apply_func(data.element_id_vertical);
            break;
        }
        case wire_new_unknown_point: {
            throw_exception("cannot add unknown point type");
            break;
        }
    };

    // delete if empty
    if (data.element_id_body == null_element && data.element_id_horizontal == null_element
        && !has_vertical_element(data)) {
        map.erase(position);
    }
    return true;
}
}  // namespace

template <class Data>
auto insert_impl(CollisionCache::map_type& map, element_id_t element_id, Data data)
    -> void {
    const auto check_empty_and_assign = [element_id](element_id_t& obj) {
        if (obj != null_element) {
            throw_exception("collision state is not empty in insert.");
        }
        obj = element_id;
    };

    iter_collision_state(data, [&](point_t position, CollisionCache::ItemType state) {
        return apply_function(map, position, state, check_empty_and_assign);
    });
}

template <class Data>
auto remove_impl(CollisionCache::map_type& map, element_id_t element_id, Data data)
    -> void {
    const auto check_and_delete = [element_id](element_id_t& obj) {
        if (obj != element_id) {
            throw_exception("exected collision state presence in remove.");
        }
        obj = null_element;
    };

    iter_collision_state(data, [&](point_t position, CollisionCache::ItemType state) {
        return apply_function(map, position, state, check_and_delete);
    });
}

template <class Data>
auto update_impl(CollisionCache::map_type& map, element_id_t new_element_id,
                 element_id_t old_element_id, Data data) -> void {
    const auto check_and_update = [new_element_id, old_element_id](element_id_t& obj) {
        if (obj != old_element_id) {
            throw_exception("exected collision state presence in update.");
        }
        obj = new_element_id;
    };

    iter_collision_state(data, [&](point_t position, CollisionCache::ItemType state) {
        return apply_function(map, position, state, check_and_update);
    });
}

auto CollisionCache::format() const -> std::string {
    // return fmt::format("CollisionCache = {}\n", map_);
    return "!!! NOT IMPLEMENTED !!!";
}

auto CollisionCache::submit(editable_circuit::InfoMessage message) -> void {
    using namespace editable_circuit::info_message;

    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        return handle(*pointer);
    }
    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        return handle(*pointer);
    }
    if (auto pointer = std::get_if<InsertedLogicItemUpdated>(&message)) {
        return handle(*pointer);
    }

    if (auto pointer = std::get_if<SegmentInserted>(&message)) {
        return handle(*pointer);
    }
    if (auto pointer = std::get_if<SegmentUninserted>(&message)) {
        return handle(*pointer);
    }
    if (auto pointer = std::get_if<InsertedSegmentUpdated>(&message)) {
        return handle(*pointer);
    }
}

auto CollisionCache::handle(editable_circuit::info_message::LogicItemInserted message)
    -> void {
    insert_impl(map_, message.element_id, message.data);
}

auto CollisionCache::handle(editable_circuit::info_message::LogicItemUninserted message)
    -> void {
    remove_impl(map_, message.element_id, message.data);
}

auto CollisionCache::handle(
    editable_circuit::info_message::InsertedLogicItemUpdated message) -> void {
    update_impl(map_, message.new_element_id, message.old_element_id, message.data);
}

auto CollisionCache::handle(editable_circuit::info_message::SegmentInserted message)
    -> void {
    insert_impl(map_, message.segment.element_id, message.segment_info);
}

auto CollisionCache::handle(editable_circuit::info_message::SegmentUninserted message)
    -> void {
    remove_impl(map_, message.segment.element_id, message.segment_info);
}

auto CollisionCache::handle(
    editable_circuit::info_message::InsertedSegmentUpdated message) -> void {
    update_impl(map_, message.new_segment.element_id, message.old_segment.element_id,
                message.segment_info);
}

auto CollisionCache::state_colliding(point_t position, ItemType item_type) const -> bool {
    if (const auto it = map_.find(position); it != map_.end()) {
        const auto data = it->second;

        switch (item_type) {
            using enum CollisionCache::ItemType;

            case element_body: {
                return true;
            }
            case element_connection: {
                return to_state(data) != CacheState::wire_connection;
            }
            case wire_connection: {
                return to_state(data) != CacheState::element_connection;
            }
            case wire_horizontal: {
                return to_state(data) != CacheState::wire_vertical;
            }
            case wire_vertical: {
                return to_state(data) != CacheState::wire_horizontal;
            }
            case wire_point: {
                return true;
            }
            case ItemType::wire_new_unknown_point: {
                const auto state = to_state(data);
                // disallow element body
                return state != CacheState::element_connection
                       && state != CacheState::wire_connection
                       && state != CacheState::wire_horizontal
                       && state != CacheState::wire_vertical
                       && state != CacheState::wire_point;
            }
        };
    }
    return false;
};

auto CollisionCache::is_colliding(layout_calculation_data_t data) const -> bool {
    return !iter_collision_state(data, [&](point_t position, ItemType state) {
        return !state_colliding(position, state);
    });
}

auto CollisionCache::get_first_wire(point_t position) const -> element_id_t {
    if (const auto it = map_.find(position); it != map_.end()) {
        const auto data = it->second;

        if (data.element_id_horizontal != null_element) {
            return data.element_id_horizontal;
        }
        if (has_vertical_element(data)) {
            return data.element_id_vertical;
        }
    }
    return null_element;
}

auto CollisionCache::creates_loop(line_t line) const -> bool {
    const auto element_id_0 = get_first_wire(line.p0);
    const auto element_id_1 = get_first_wire(line.p1);

    return element_id_0 != null_element && element_id_0 == element_id_1;
}

auto CollisionCache::is_colliding(line_t line) const -> bool {
    if (creates_loop(line)) {
        return true;
    }

    const auto segment = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::new_unknown,
        .p1_type = SegmentPointType::new_unknown,
    };

    return !iter_collision_state(segment, [&](point_t position, ItemType state) {
        return !state_colliding(position, state);
    });
}

auto CollisionCache::to_state(collision_data_t data) -> CacheState {
    using enum CacheState;

    if (data.element_id_body != null_element  //
        && data.element_id_horizontal == null_element
        && data.element_id_vertical == null_element) {
        return element_body;
    }

    if (data.element_id_body != null_element  //
        && data.element_id_horizontal == null_element
        && data.element_id_vertical == connection_tag) {
        return element_connection;
    }

    if (data.element_id_body == null_element           //
        && data.element_id_horizontal != null_element  //
        && data.element_id_vertical == connection_tag) {
        return wire_connection;
    }

    if (data.element_id_body == null_element  //
        && data.element_id_horizontal != null_element
        && data.element_id_vertical == null_element) {
        return wire_horizontal;
    }

    if (data.element_id_body == null_element           //
        && data.element_id_horizontal == null_element  //
        && has_vertical_element(data)) {
        return wire_vertical;
    }

    if (data.element_id_body == null_element           //
        && data.element_id_horizontal != null_element  //
        && has_vertical_element(data)
        && data.element_id_horizontal == data.element_id_vertical) {
        return wire_point;
    }

    // inferred states -> two elements

    if (data.element_id_body == null_element           //
        && data.element_id_horizontal != null_element  //
        && has_vertical_element(data)
        && data.element_id_horizontal != data.element_id_vertical) {
        return wire_crossing;
    }

    if (data.element_id_body != null_element  //
        && data.element_id_horizontal != null_element
        && data.element_id_vertical == connection_tag) {
        return element_wire_connection;
    }

    // return invalid state, so checking for states compiles efficiently
    return invalid_state;
}

auto CollisionCache::validate(const Circuit& circuit) const -> void {
    auto cache = CollisionCache {};
    add_circuit_to_cache(cache, circuit);

    if (cache.map_ != this->map_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

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
