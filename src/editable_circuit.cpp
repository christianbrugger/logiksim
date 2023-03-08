#include "editable_circuit.h"

#include "algorithm.h"
#include "exceptions.h"
#include "geometry.h"
#include "iterator_adaptor.h"
#include "layout_calculations.h"
#include "range.h"
#include "scene.h"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>

namespace logicsim {

using delete_queue_t = folly::small_vector<element_id_t, 6>;

auto orientations_compatible(orientation_t a, orientation_t b) -> bool {
    using enum orientation_t;
    return (a == left && b == right) || (a == right && b == left)
           || (a == up && b == down) || (a == down && b == up) || (a == undirected)
           || (b == undirected);
}

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
auto ConnectionCache<IsInput>::insert(element_id_t element_id,
                                      layout_calculation_data_t data) -> void {
    // placeholders are not cached
    if (is_placeholder(data)) {
        return;
    }

    const auto add_position = [&](connection_id_t connection_id, point_t position,
                                  orientation_t orientation) {
        if (connections_.contains(position)) [[unlikely]] {
            throw_exception("cache already has an entry at this position");
        }
        connections_[position] = {element_id, connection_id, orientation};
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(data, add_position);
    } else {
        iter_output_location_and_id(data, add_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::remove(element_id_t element_id,
                                      layout_calculation_data_t data) -> void {
    // placeholders are not cached
    if (is_placeholder(data)) {
        return;
    }

    const auto remove_position = [&](connection_id_t connection_id, point_t position,
                                     orientation_t orientation) {
        const auto value = value_type {element_id, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(connections_, position, value);
        connections_.erase(it);
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(data, remove_position);
    } else {
        iter_output_location_and_id(data, remove_position);
    }
}

template <bool IsInput>
auto ConnectionCache<IsInput>::update(element_id_t new_element_id,
                                      element_id_t old_element_id,
                                      layout_calculation_data_t data) -> void {
    // placeholders are not cached
    if (is_placeholder(data)) {
        return;
    }

    const auto update_id = [&](connection_id_t connection_id, point_t position,
                               orientation_t orientation) {
        const auto old_value = value_type {old_element_id, connection_id, orientation};
        const auto it = get_and_verify_cache_entry(connections_, position, old_value);
        it->second.element_id = new_element_id;
        return true;
    };

    if constexpr (IsInput) {
        iter_input_location_and_id(data, update_id);
    } else {
        iter_output_location_and_id(data, update_id);
    }
}

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

//
// Collision Cache
//

namespace {
[[nodiscard]] auto has_vertical_element(CollisionCache::collision_data_t data) -> bool {
    return data.element_id_vertical >= element_id_t {0};
}

}  // namespace

// next_state(point_t position, CollisionState state) -> bool
template <typename Func>
auto iter_segment_collision_state(const LineTree& line_tree, Func next_state) -> bool {
    for (auto segment : line_tree.segments()) {
        auto line = order_points(line_t {segment.p0, segment.p1});

        if (is_horizontal(line)) {
            for (auto x : range(line.p0.x + grid_t {1}, line.p1.x)) {
                if (!next_state(point_t {x, line.p0.y},
                                CollisionCache::CollisionState::wire_horizontal)) {
                    return false;
                }
            }
        } else {
            for (auto y : range(line.p0.y + grid_t {1}, line.p1.y)) {
                if (!next_state(point_t {line.p0.x, y},
                                CollisionCache::CollisionState::wire_vertical)) {
                    return false;
                }
            }
        }
    }
    return true;
}

// next_state(point_t position, CollisionState state) -> bool
template <typename Func>
auto iter_wire_collision_state(layout_calculation_data_t data, Func next_state) -> bool {
    const auto& line_tree = data.line_tree;

    if (line_tree.empty()) {
        return true;
    }

    if (!iter_segment_collision_state(line_tree, next_state)) {
        return false;
    }

    if (!next_state(line_tree.input_position(),
                    CollisionCache::CollisionState::wire_connection)) {
        return false;
    }

    for (point_t point : line_tree.internal_points()) {
        if (!next_state(point, CollisionCache::CollisionState::wire_point)) {
            return false;
        }
    }

    for (point_t point : line_tree.output_positions()) {
        if (!next_state(point, CollisionCache::CollisionState::wire_connection)) {
            return false;
        }
    }

    return true;
}

// next_state(point_t position, CollisionState state) -> bool
template <typename Func>
auto iter_body_collision_state(layout_calculation_data_t data, Func next_state) -> bool {
    if (!iter_input_location(
            data, [=](point_t position, orientation_t _ [[maybe_unused]]) {
                return next_state(position,
                                  CollisionCache::CollisionState::element_connection);
            })) {
        return false;
    }

    if (!iter_element_body_points(data, [=](point_t position) {
            return next_state(position, CollisionCache::CollisionState::element_body);
        })) {
        return false;
    }

    if (!iter_output_location(
            data, [=](point_t position, orientation_t _ [[maybe_unused]]) {
                return next_state(position,
                                  CollisionCache::CollisionState::element_connection);
            })) {
        return false;
    }

    return true;
}

// next_state(point_t position, CollisionState state) -> bool
template <typename Func>
auto iter_collision_state(layout_calculation_data_t data, Func next_state) -> bool {
    if (data.element_type == ElementType::placeholder) {
        return true;
    } else if (data.element_type == ElementType::wire) {
        return iter_wire_collision_state(data, next_state);
    }
    return iter_body_collision_state(data, next_state);
}

namespace {
// apply_func(element_id_t& obj) -> void
template <typename Apply>
auto apply_function(CollisionCache::map_type& map, point_t position,
                    CollisionCache::CollisionState state, Apply apply_func) -> bool {
    auto& data = map[position];

    const auto set_connection_tag = [&]() {
        if (has_vertical_element(data)) {
            throw_exception("cannot set connection tag, second element occupied");
        }
        data.element_id_vertical = CollisionCache::connection_tag;
    };

    switch (state) {
        using enum CollisionCache::CollisionState;

        case element_body: {
            apply_func(data.element_id_body);
            break;
        }
        case element_connection: {
            set_connection_tag();
            apply_func(data.element_id_body);
            break;
        }
        case wire_connection:
            set_connection_tag();
            apply_func(data.element_id_horizontal);
            break;
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
        case wire_crossing:
        case element_wire_connection:
        case invalid_state: {
            throw_exception("infered states are invalid when iterating element");
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

auto CollisionCache::insert(element_id_t element_id, layout_calculation_data_t data)
    -> void {
    const auto check_empty_and_assign = [element_id](element_id_t& obj) {
        if (obj != null_element) {
            throw_exception("collision state is not empty in insert.");
        }
        obj = element_id;
    };

    iter_collision_state(data, [&](point_t position, CollisionState state) {
        return apply_function(map_, position, state, check_empty_and_assign);
    });
}

auto CollisionCache::remove(element_id_t element_id, layout_calculation_data_t data)
    -> void {
    const auto check_and_delete = [element_id](element_id_t& obj) {
        if (obj != element_id) {
            throw_exception("exected collision state presence in remove.");
        }
        obj = null_element;
    };

    iter_collision_state(data, [&](point_t position, CollisionState state) {
        return apply_function(map_, position, state, check_and_delete);
    });
}

auto CollisionCache::update(element_id_t new_element_id, element_id_t old_element_id,
                            layout_calculation_data_t data) -> void {
    const auto check_and_update = [new_element_id, old_element_id](element_id_t& obj) {
        if (obj != old_element_id) {
            throw_exception("exected collision state presence in update.");
        }
        obj = new_element_id;
    };

    iter_collision_state(data, [&](point_t position, CollisionState state) {
        return apply_function(map_, position, state, check_and_update);
    });
}

auto CollisionCache::is_colliding(layout_calculation_data_t data) const -> bool {
    const auto not_colliding = [&](point_t position, CollisionState state) {
        if (const auto it = map_.find(position); it != map_.end()) {
            const auto data = it->second;

            switch (state) {
                using enum CollisionCache::CollisionState;

                case element_body: {
                    return false;
                }
                case element_connection: {
                    return to_state(data) == wire_connection;
                }
                case wire_connection: {
                    return to_state(data) == element_connection;
                }
                case wire_horizontal: {
                    return to_state(data) == wire_vertical;
                }
                case wire_vertical: {
                    return to_state(data) == wire_horizontal;
                }
                case wire_point: {
                    return false;
                }
                case wire_crossing:
                case element_wire_connection:
                case invalid_state: {
                    throw_exception("infered states are invalid when iterating element");
                }
            };
        }
        return true;
    };

    return !iter_collision_state(data, not_colliding);
}

auto CollisionCache::to_state(collision_data_t data) -> CollisionState {
    using enum CollisionState;

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

//
// ElementKeyStore
//

auto ElementKeyStore::insert(element_id_t element_id) -> element_key_t {
    const auto element_key = next_key_;
    next_key_.value += 1;

    if (bool was_inserted = map_to_key_.insert({element_id, element_key}).second;
        !was_inserted) [[unlikely]] {
        throw_exception("Element id already exists in key store.");
    }

    if (bool was_inserted = map_to_id_.insert({element_key, element_id}).second;
        !was_inserted) [[unlikely]] {
        throw_exception("Element key already exists in key store.");
    }

    return element_key;
}

auto ElementKeyStore::remove(element_id_t element_id) -> void {
    const auto it1 = map_to_key_.find(element_id);
    if (it1 == map_to_key_.end()) [[unlikely]] {
        throw_exception("Cannot find element_id in key store.");
    }

    const auto element_key = it1->second;
    const auto it2 = map_to_id_.find(element_key);
    if (it2 == map_to_id_.end()) [[unlikely]] {
        throw_exception("Cannot find element_key in key store.");
    }

    map_to_key_.erase(it1);
    map_to_id_.erase(it2);
}

auto ElementKeyStore::update(element_id_t new_element_id, element_id_t old_element_id)
    -> void {
    const auto it1 = map_to_key_.find(old_element_id);
    if (it1 == map_to_key_.end()) [[unlikely]] {
        throw_exception("Cannot find old_element_id in key store.");
    }
    const auto element_key = it1->second;

    const auto it2 = map_to_id_.find(element_key);
    if (it2 == map_to_id_.end()) [[unlikely]] {
        throw_exception("Cannot find element_key in key store.");
    }

    // update
    it2->second = new_element_id;
    map_to_key_.erase(it1);
    if (bool was_inserted = map_to_key_.insert({new_element_id, element_key}).second;
        !was_inserted) [[unlikely]] {
        throw_exception("New element id was still part of cache.");
    }
}

auto ElementKeyStore::to_element_id(element_key_t element_key) const -> element_id_t {
    if (element_key < element_key_t {0} || element_key >= next_key_) [[unlikely]] {
        throw_exception("Invalid element key.");
    }

    const auto it = map_to_id_.find(element_key);
    return (it == map_to_id_.end()) ? null_element : it->second;
}

auto ElementKeyStore::to_element_key(element_id_t element_id) const -> element_key_t {
    const auto it = map_to_key_.find(element_id);

    if (it == map_to_key_.end()) [[unlikely]] {
        throw_exception("Element id not found in key store.");
    }

    return it->second;
}

auto ElementKeyStore::size() const -> std::size_t {
    if (map_to_id_.size() != map_to_key_.size()) [[unlikely]] {
        throw_exception("maps have different sizes");
    }
    return map_to_id_.size();
}

//
// Editable Circuit
//

EditableCircuit::EditableCircuit(Schematic&& schematic, Layout&& layout)
    : schematic_ {std::move(schematic)}, layout_ {std::move(layout)} {}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}\n{}\n}}", schematic_, layout_);
}

auto EditableCircuit::layout() const noexcept -> const Layout& {
    return layout_;
}

auto EditableCircuit::schematic() const noexcept -> const Schematic& {
    return schematic_;
}

auto EditableCircuit::add_placeholder_element() -> element_id_t {
    constexpr static auto connector_delay
        = delay_t {Schematic::defaults::wire_delay_per_distance.value / 2};

    const auto element_id = layout_.add_placeholder();
    {
        const auto element = schematic_.add_element(Schematic::NewElementData {
            .element_type = ElementType::placeholder,
            .input_count = 1,
            .output_count = 0,
            .history_length = connector_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    return element_id;
}

auto EditableCircuit::add_inverter_element(point_t position, InsertionMode insertion_mode,
                                           orientation_t orientation) -> element_key_t {
    return add_standard_element(ElementType::inverter_element, 1, position,
                                insertion_mode, orientation);
}

auto EditableCircuit::add_standard_element(ElementType type, std::size_t input_count,
                                           point_t position, InsertionMode insertion_mode,
                                           orientation_t orientation) -> element_key_t {
    using enum ElementType;
    if (!(type == and_element || type == or_element || type == xor_element
          || type == inverter_element)) [[unlikely]] {
        throw_exception("The type needs to be a standard element.");
    }
    if (type == inverter_element && input_count != 1) [[unlikely]] {
        throw_exception("Inverter needs to have exactly one input.");
    }
    if (type != inverter_element && input_count < 2) [[unlikely]] {
        throw_exception("Input count needs to be at least 2 for standard elements.");
    }

    const static auto empty_line_tree = LineTree {};
    const auto data = layout_calculation_data_t {
        .line_tree = empty_line_tree,
        .input_count = input_count,
        .output_count = 1,
        .internal_state_count = 0,
        .position = position,
        .orientation = orientation,
        .element_type = type,
    };
    if (!is_representable_(data)) {
        return null_element_key;
    }

    // check for collisions
    const bool colliding = [&]() {
        if (insertion_mode == InsertionMode::temporary) {
            return false;
        }

        return is_colliding(data);
    }();

    if (insertion_mode == InsertionMode::insert_or_discard && colliding) {
        return null_element_key;
    }

    const auto display_state = [&]() {
        switch (insertion_mode) {
            using enum InsertionMode;

            case insert_or_discard:
                return DisplayState::normal;

            case collisions:
                if (colliding) {
                    return DisplayState::new_colliding;
                } else {
                    return DisplayState::new_valid;
                }

            case temporary:
                return DisplayState::new_temporary;
        };
        throw_exception("unknown mode");
    }();

    // insert into underlyings
    auto element_id = layout_.add_logic_element(position, orientation, display_state);
    {
        const auto element = schematic_.add_element({
            .element_type = type,
            .input_count = input_count,
            .output_count = 1,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    const auto element_key = key_insert(element_id);

    // connect
    if (is_display_state_cached(display_state)) {
        // TODO rename to better name
        connect_new_element(element_id);
    }

    return element_key;
}

auto EditableCircuit::add_wire(LineTree&& line_tree) -> element_key_t {
    const auto delays = calculate_output_delays(line_tree);
    const auto max_delay = std::ranges::max(delays);
    const auto output_count = delays.size();

    // check for collisions
    {
        const static auto empty_line_tree = LineTree {};
        const auto data = layout_calculation_data_t {
            .line_tree = line_tree,
            .input_count = 1,
            .output_count = output_count,
            .internal_state_count = 0,
            .position = {0, 0},
            .orientation = orientation_t::undirected,
            .element_type = ElementType::wire,
        };
        if (is_colliding(data)) {
            return null_element_key;
        }
    }

    // insert into underlyings
    auto element_id = layout_.add_wire(std::move(line_tree));
    {
        const auto element = schematic_.add_element({
            .element_type = ElementType::wire,
            .input_count = 1,
            .output_count = output_count,
            .output_delays = delays,
            .history_length = max_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }
    const auto element_key = key_insert(element_id);

    // connect
    connect_new_element(element_id);
    return element_key;
}

auto EditableCircuit::swap_and_delete_element(element_id_t element_id) -> void {
    if (schematic_.element(element_id).is_placeholder()) {
        throw_exception("cannot directly delete placeholders.");
    }

    auto delete_queue = delete_queue_t {element_id};

    const auto is_placeholder = [](Schematic::Output output) {
        return output.has_connected_element()
               && output.connected_element().element_type() == ElementType::placeholder;
    };
    transform_if(schematic_.element(element_id).outputs(),
                 std::back_inserter(delete_queue),
                 &Schematic::Output::connected_element_id, is_placeholder);

    swap_and_delete_multiple_elements(delete_queue);
}

auto EditableCircuit::to_element_id(element_key_t element_key) const -> element_id_t {
    return element_keys_.to_element_id(element_key);
}

auto EditableCircuit::to_element_key(element_id_t element_id) const -> element_key_t {
    return element_keys_.to_element_key(element_id);
}

auto EditableCircuit::swap_and_delete_multiple_elements(
    std::span<const element_id_t> element_ids) -> void {
    // sort descending, so we don't invalidate our ids
    auto sorted_ids = delete_queue_t {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (const auto element_id : sorted_ids) {
        swap_and_delete_single_element(element_id);
    }
}

auto EditableCircuit::swap_and_delete_single_element(element_id_t element_id) -> void {
    const auto is_cached = is_display_state_cached(layout_.display_state(element_id));
    key_remove(element_id);
    if (is_cached) {
        cache_remove(element_id);
    }

    // delete in underlying
    auto last_id1 = schematic_.swap_and_delete_element(element_id);
    auto last_id2 = layout_.swap_and_delete_element(element_id);
    if (last_id1 != last_id2) {
        throw_exception("Returned id's during deletion are not the same.");
    }

    // update ids
    if (element_id != last_id1) {
        key_update(element_id, last_id1);
        if (is_cached) {
            cache_update(element_id, last_id1);
        }
    }
}

auto EditableCircuit::add_missing_placeholders(element_id_t element_id) -> void {
    for (const auto output : schematic_.element(element_id).outputs()) {
        if (!output.has_connected_element()) {
            auto placeholder_id = add_placeholder_element();
            output.connect(schematic_.element(placeholder_id).input(connection_id_t {0}));
        }
    }
}

namespace {
struct connector_data_t {
    // element_id and connection_id
    connection_t connection_data;
    // position of the connector
    point_t position;
    // orientation of the connector
    orientation_t orientation;
};
}  // namespace

template <bool IsInput>
auto connect_connector(connector_data_t connector,
                       ConnectionCache<IsInput>& connection_cache, Schematic& schematic)
    -> std::optional<element_id_t> {
    auto unused_placeholder_id = std::optional<element_id_t> {};
    auto connection = to_connection<!IsInput>(schematic, connector.connection_data);

    // pre-conditions
    if (connection.has_connected_element()) [[unlikely]] {
        throw_exception("Connections needs to be unconnected.");
    }

    // find connection at position
    if (const auto entry = connection_cache.find(connector.position, schematic)) {
        const auto found_connection = entry->first;
        const auto found_orientation = entry->second;

        if (found_connection.has_connected_element()) {
            if (!found_connection.connected_element().is_placeholder()) [[unlikely]] {
                throw_exception("Connection is already connected at this location.");
            }
            // mark placeholder for deletion
            unused_placeholder_id = found_connection.connected_element_id();
        }
        if (!orientations_compatible(connector.orientation, found_orientation)) {
            throw_exception("Connection have incompatible orientations.");
        }

        // make connection in schematic
        connection.connect(found_connection);
    }

    return unused_placeholder_id;
}

auto EditableCircuit::connect_new_element(element_id_t& element_id) -> void {
    auto delete_queue = delete_queue_t {};
    auto add_if_valid = [&](std::optional<element_id_t> placeholder_id) {
        if (placeholder_id) {
            delete_queue.push_back(*placeholder_id);
        }
    };

    const auto data = to_layout_calculation_data(schematic_, layout_, element_id);

    // inputs
    iter_input_location_and_id(
        data, [&, element_id](connection_id_t input_id, point_t position,
                              orientation_t orientation) {
            auto input = connector_data_t {{element_id, input_id}, position, orientation};
            // connect the input using the output_connections cache
            const auto placeholder_id
                = connect_connector(input, output_connections_, schematic_);
            add_if_valid(placeholder_id);
            return true;
        });

    // outputs
    iter_output_location_and_id(data, [&, element_id](connection_id_t output_id,
                                                      point_t position,
                                                      orientation_t orientation) mutable {
        auto output = connector_data_t {{element_id, output_id}, position, orientation};
        // connect the output using the input_connections cache
        const auto placeholder_id
            = connect_connector(output, input_connections_, schematic_);
        add_if_valid(placeholder_id);
        return true;
    });

    cache_insert(element_id);
    add_missing_placeholders(element_id);

    // this invalidates our element_id
    swap_and_delete_multiple_elements(delete_queue);
    element_id = null_element;
}

auto EditableCircuit::is_representable_(layout_calculation_data_t data) const -> bool {
    if (is_placeholder(data)) {
        return true;
    }
    if (data.element_type == ElementType::wire) {
        return true;
    }

    const auto position = data.position;
    data.position = point_t {0, 0};
    const auto rect = element_collision_rect(data);

    return is_representable(position.x.value + rect.p0.x.value,
                            position.y.value + rect.p0.y.value)
           && is_representable(position.x.value + rect.p1.x.value,
                               position.y.value + rect.p1.y.value);
}

auto EditableCircuit::is_colliding(layout_calculation_data_t data) const -> bool {
    return collicions_cache_.is_colliding(data) || input_connections_.is_colliding(data)
           || output_connections_.is_colliding(data);
}

auto EditableCircuit::is_display_state_cached(DisplayState display_state) const -> bool {
    return display_state == DisplayState::normal
           || display_state == DisplayState::new_valid;
}

auto EditableCircuit::key_insert(element_id_t element_id) -> element_key_t {
    if (schematic_.element(element_id).is_placeholder()) {
        return null_element_key;
    }

    return element_keys_.insert(element_id);
}

auto EditableCircuit::key_remove(element_id_t element_id) -> void {
    if (schematic_.element(element_id).is_placeholder()) {
        return;
    }
    element_keys_.remove(element_id);
}

auto EditableCircuit::key_update(element_id_t new_element_id, element_id_t old_element_id)
    -> void {
    if (schematic_.element(new_element_id).is_placeholder()) {
        return;
    }
    element_keys_.update(new_element_id, old_element_id);
}

auto EditableCircuit::cache_insert(element_id_t element_id) -> void {
    const auto data = to_layout_calculation_data(schematic_, layout_, element_id);

    input_connections_.insert(element_id, data);
    output_connections_.insert(element_id, data);
    collicions_cache_.insert(element_id, data);
    selection_cache_.insert(element_id, data);
}

auto EditableCircuit::cache_remove(element_id_t element_id) -> void {
    const auto data = to_layout_calculation_data(schematic_, layout_, element_id);

    input_connections_.remove(element_id, data);
    output_connections_.remove(element_id, data);
    collicions_cache_.remove(element_id, data);
    selection_cache_.remove(element_id, data);
}

auto EditableCircuit::cache_update(element_id_t new_element_id,
                                   element_id_t old_element_id) -> void {
    const auto data = to_layout_calculation_data(schematic_, layout_, new_element_id);

    input_connections_.update(new_element_id, old_element_id, data);
    output_connections_.update(new_element_id, old_element_id, data);
    collicions_cache_.update(new_element_id, old_element_id, data);
    selection_cache_.update(new_element_id, old_element_id, data);
}

}  // namespace logicsim
