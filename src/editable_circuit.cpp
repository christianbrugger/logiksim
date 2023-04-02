#include "editable_circuit.h"

#include "algorithm.h"
#include "circuit.h"
#include "circuit_index.h"
#include "exceptions.h"
#include "geometry.h"
#include "iterator_adaptor.h"
#include "layout_calculations.h"
#include "range.h"
#include "scene.h"
#include "selection_handle.h"

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

auto add_circuit_to_cache(auto&& cache, const Layout& layout, const Schematic& schematic)
    -> void {
    iter_circuit_elements(
        layout, schematic,
        [&cache](element_id_t element_id, layout_calculation_data_t data) {
            cache.insert(element_id, data);
        },
        [&cache](element_id_t element_id, segment_info_t segment,
                 segment_index_t segment_index) { cache.insert(element_id, segment); });
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
auto ConnectionCache<IsInput>::insert(element_id_t element_id, segment_info_t segment)
    -> void {}

template <bool IsInput>
auto ConnectionCache<IsInput>::remove(element_id_t element_id, segment_info_t segment)
    -> void {}

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
auto ConnectionCache<IsInput>::validate(const Layout& layout,
                                        const Schematic& schematic) const -> void {
    auto cache = ConnectionCache<IsInput> {};
    add_circuit_to_cache(cache, layout, schematic);

    if (cache.connections_ != this->connections_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
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

auto CollisionCache::insert(element_id_t element_id, layout_calculation_data_t data)
    -> void {
    insert_impl(map_, element_id, data);
}

auto CollisionCache::remove(element_id_t element_id, layout_calculation_data_t data)
    -> void {
    remove_impl(map_, element_id, data);
}

auto CollisionCache::update(element_id_t new_element_id, element_id_t old_element_id,
                            layout_calculation_data_t data) -> void {
    if (data.element_type == ElementType::wire) {
        for (auto&& segment : data.segment_tree.segments()) {
            update_impl(map_, new_element_id, old_element_id, segment);
        }
    } else {
        update_impl(map_, new_element_id, old_element_id, data);
    }
}

auto CollisionCache::insert(element_id_t element_id, segment_info_t segment) -> void {
    insert_impl(map_, element_id, segment);
}

auto CollisionCache::remove(element_id_t element_id, segment_info_t segment) -> void {
    remove_impl(map_, element_id, segment);
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

auto CollisionCache::validate(const Layout& layout, const Schematic& schematic) const
    -> void {
    auto cache = CollisionCache {};
    add_circuit_to_cache(cache, layout, schematic);

    if (cache.map_ != this->map_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

//
// Editable Circuit
//

EditableCircuit::EditableCircuit(Schematic&& schematic, Layout&& layout)
    : selection_builder_ {*this},
      schematic_ {std::move(schematic)},
      layout_ {std::move(layout)} {
    // TODO consider bulk insertion, especially for spatial_cache_
    add_circuit_to_cache(input_connections_, layout_, schematic_);
    add_circuit_to_cache(output_connections_, layout_, schematic_);
    add_circuit_to_cache(collision_cache_, layout_, schematic_);
    add_circuit_to_cache(spatial_cache_, layout_, schematic_);
}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}\n{}\n}}", schematic_, layout_);
}

auto EditableCircuit::layout() const noexcept -> const Layout& {
    return layout_;
}

auto EditableCircuit::schematic() const noexcept -> const Schematic& {
    return schematic_;
}

auto to_insertion_mode(display_state_t display_state) -> InsertionMode {
    switch (display_state) {
        using enum display_state_t;

        case normal:
            return InsertionMode::insert_or_discard;
        case new_colliding:
            return InsertionMode::collisions;
        case new_valid:
            return InsertionMode::collisions;
        case new_temporary:
            return InsertionMode::temporary;
    };

    throw_exception("Unknown display state.");
};

auto to_display_state(InsertionMode insertion_mode, bool is_colliding)
    -> display_state_t {
    switch (insertion_mode) {
        using enum InsertionMode;

        case insert_or_discard:
            return display_state_t::normal;

        case collisions:
            if (is_colliding) {
                return display_state_t::new_colliding;
            } else {
                return display_state_t::new_valid;
            }

        case temporary:
            return display_state_t::new_temporary;
    };
    throw_exception("unknown insertion mode");
}

auto EditableCircuit::validate() -> void {
    // validates layout & schematic
    logicsim::validate(layout_, schematic_);

    // caches
    spatial_cache_.validate(layout_, schematic_);
    collision_cache_.validate(layout_, schematic_);
    input_connections_.validate(layout_, schematic_);
    output_connections_.validate(layout_, schematic_);

    // selections
    for (const auto& item : managed_selections_) {
        if (item.second == nullptr) [[unlikely]] {
            throw_exception("selection cannot be nullptr");
        }
        item.second->validate(layout_, schematic_);
    }
    selection_builder_.validate(layout_, schematic_);
}

auto EditableCircuit::add_placeholder_element() -> element_id_t {
    constexpr static auto connector_delay
        = delay_t {Schematic::defaults::wire_delay_per_distance.value / 2};

    const auto element_id = layout_.add_placeholder(display_state_t::normal);
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
                                           orientation_t orientation)
    -> selection_handle_t {
    return add_standard_element(ElementType::inverter_element, 1, position,
                                insertion_mode, orientation);
}

auto EditableCircuit::add_standard_element(ElementType type, std::size_t input_count,
                                           point_t position, InsertionMode insertion_mode,
                                           orientation_t orientation)
    -> selection_handle_t {
    using enum ElementType;
    if (!(type == and_element || type == or_element || type == xor_element
          || type == inverter_element)) [[unlikely]] {
        throw_exception("The type needs to be a standard element.");
    }
    if (type == inverter_element && input_count != 1) [[unlikely]] {
        throw_exception("Inverter needs to have exactly one input.");
    }
    if (type != inverter_element && input_count < 2) [[unlikely]] {
        throw_exception(
            "Input count needs to be at least 2 for "
            "standard elements.");
    }
    auto selection_handle = create_selection();

    // insert into underlyings
    auto element_id = layout_.add_logic_element(point_t {0, 0}, orientation,
                                                display_state_t::new_temporary);
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
    key_insert(element_id);
    selection_handle.value().add_element(element_id);

    // validates our position
    move_or_delete_element(element_id, position.x.value, position.y.value);
    if (element_id) {
        change_insertion_mode(element_id, insertion_mode);
    }
    return selection_handle;
}

auto get_segment(const Layout& layout, segment_t segment) -> segment_info_t {
    return layout.segment_tree(segment.element_id).segment(segment.segment_index);
}

auto get_segment_line(const Layout& layout, segment_t segment) -> line_t {
    return get_segment(layout, segment).line;
}

auto EditableCircuit::set_segment_point_types(
    std::initializer_list<const std::pair<segment_t, SegmentPointType>> data,
    point_t position) -> void {
    // remove cache
    for (auto&& [segment, point_type] : data) {
        // TODO only do this for some insertion modes
        cache_remove(segment.element_id, segment.segment_index);
    }

    // update segments
    for (auto&& [segment, point_type] : data) {
        auto&& m_tree = layout_.modifyable_segment_tree(segment.element_id);
        auto new_segment = m_tree.segment(segment.segment_index);

        if (new_segment.line.p0 == position) {
            new_segment.p0_type = point_type;
        } else if (new_segment.line.p1 == position) {
            new_segment.p1_type = point_type;
        } else {
            throw_exception("Position needs to be an endpoint of the given segment.");
        }

        m_tree.update_segment(segment.segment_index, new_segment,
                              display_state_t::normal);
    }

    // add to cache
    for (auto&& [segment, point_type] : data) {
        // TODO only do this for some insertion modes
        cache_insert(segment.element_id, segment.segment_index);
    }
}

auto sort_lines_with_endpoints_last(std::span<std::pair<line_t, segment_t>> lines,
                                    point_t point) -> void {
    std::ranges::sort(lines, {}, [point](std::pair<line_t, segment_t> item) {
        return is_endpoint(point, item.first);
    });
}

auto merge_parallel_segments(segment_info_t segment_info_0, segment_info_t segment_info_1)
    -> segment_info_t {
    const auto [a, b] = order_points(segment_info_0, segment_info_1);

    if (a.line.p1 != b.line.p0) [[unlikely]] {
        throw_exception("segments need to have common shared point");
    }

    return segment_info_t {
        .line = line_t {a.line.p0, b.line.p1},
        .p0_type = a.p0_type,
        .p1_type = b.p1_type,
    };
}

auto EditableCircuit::merge_line_segments(element_id_t element_id, segment_index_t index0,
                                          segment_index_t index1) -> void {
    if (index0 == index1) [[unlikely]] {
        throw_exception("Cannot merge the same segments.");
    }
    if (index0 > index1) {
        std::swap(index0, index1);
    }

    auto& m_tree = layout_.modifyable_segment_tree(element_id);
    const auto last_index = m_tree.last_index();

    if (!is_inserted(m_tree.display_state(index0))
        || !is_inserted(m_tree.display_state(index1))) [[unlikely]] {
        throw_exception("Can only merge inserted segments.");
    }

    // merged segment
    const auto merged_segment
        = merge_parallel_segments(m_tree.segment(index0), m_tree.segment(index1));

    // remove from cache
    cache_remove(element_id, index0);
    cache_remove(element_id, index1);
    if (index1 != last_index) {
        cache_remove(element_id, last_index);
    }

    // merge
    m_tree.update_segment(index0, merged_segment, display_state_t::normal);
    m_tree.swap_and_delete_segment(index1);

    // add back to cache
    cache_insert(element_id, index0);
    if (index1 != last_index) {
        cache_insert(element_id, index1);
    }
}

auto all_collision_condered(const SegmentTree& tree,
                            SearchTree::queried_segments_t result) -> bool {
    return std::ranges::all_of(result, [&](segment_t value) {
        return value.segment_index == null_segment_index
               || is_inserted(tree.display_state(value.segment_index));
    });
}

auto EditableCircuit::fix_line_segments(point_t position) -> void {
    // TODO rename to segments
    const auto segment = spatial_cache_.query_line_segments(position);
    const auto segment_count = get_segment_count(segment);

    if (segment_count == 0) [[unlikely]] {
        throw_exception("Could not find any segments at position.");
    }
    if (!all_same_element_id(segment)) [[unlikely]] {
        throw_exception("All segments need to belong to the same segment tree.");
    }
    if (const auto tree = layout_.segment_tree(segment.at(0).element_id);
        !all_collision_condered(tree, segment)) {
        throw_exception("Can only fix collision considered segments.");
    }

    if (segment_count == 1) {
        set_segment_point_types(
            {
                std::pair {segment.at(0), SegmentPointType::output},
            },
            position);
        return;
    }

    if (segment_count == 2) {
        auto lines = std::array {
            std::pair {get_segment_line(layout_, segment.at(0)), segment.at(0)},
            std::pair {get_segment_line(layout_, segment.at(1)), segment.at(1)},
        };
        sort_lines_with_endpoints_last(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            const auto cross_point_type = is_horizontal(lines.at(1).first)
                                              ? SegmentPointType::cross_point_horizontal
                                              : SegmentPointType::cross_point_vertical;
            set_segment_point_types(
                {
                    std::pair {lines.at(1).second, cross_point_type},
                },
                position);
            return;
        }

        const auto horizontal_0 = is_horizontal(lines.at(0).first);
        const auto horizontal_1 = is_horizontal(lines.at(1).first);
        const auto parallel = horizontal_0 == horizontal_1;

        if (parallel) {
            merge_line_segments(segment.at(0).element_id, segment.at(0).segment_index,
                                segment.at(1).segment_index);
            return;
        }

        // handle corner
        set_segment_point_types(
            {
                std::pair {segment.at(0), SegmentPointType::colliding_point},
                std::pair {segment.at(1), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    if (segment_count == 3) {
        auto lines = std::array {
            std::pair {get_segment_line(layout_, segment.at(0)), segment.at(0)},
            std::pair {get_segment_line(layout_, segment.at(1)), segment.at(1)},
            std::pair {get_segment_line(layout_, segment.at(2)), segment.at(2)},
        };
        sort_lines_with_endpoints_last(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            const auto cross_point_type = is_horizontal(lines.at(2).first)
                                              ? SegmentPointType::cross_point_horizontal
                                              : SegmentPointType::cross_point_vertical;
            set_segment_point_types(
                {
                    std::pair {lines.at(1).second, SegmentPointType::shadow_point},
                    std::pair {lines.at(2).second, cross_point_type},
                },
                position);
        } else {
            set_segment_point_types(
                {
                    std::pair {segment.at(0), SegmentPointType::colliding_point},
                    std::pair {segment.at(1), SegmentPointType::shadow_point},
                    std::pair {segment.at(2), SegmentPointType::visual_cross_point},
                },
                position);
        }
        return;
    }

    if (segment_count == 4) {
        set_segment_point_types(
            {
                std::pair {segment.at(0), SegmentPointType::colliding_point},
                std::pair {segment.at(1), SegmentPointType::shadow_point},
                std::pair {segment.at(2), SegmentPointType::shadow_point},
                std::pair {segment.at(3), SegmentPointType::visual_cross_point},
            },
            position);
        return;
    }

    throw_exception("unexpected unhandeled case");
}

auto EditableCircuit::add_line_segment(line_t line, InsertionMode insertion_mode,
                                       Selection* selection) -> void {
    if (insertion_mode != InsertionMode::insert_or_discard) {
        throw_exception("Not implemented.");
    }

    if (is_colliding(line)) {
        fmt::print("Collision failed\n");
        return;
    }

    const auto colliding_id_0 = collision_cache_.get_first_wire(line.p0);
    auto colliding_id_1 = collision_cache_.get_first_wire(line.p1);

    auto tree_handle = element_handle();

    if (colliding_id_0) {
        tree_handle.set_element(colliding_id_0);
    }

    if (colliding_id_1) {
        if (!tree_handle) {
            tree_handle.set_element(colliding_id_1);
        } else {
            // merge two trees
            const auto tree_copy = SegmentTree {layout_.segment_tree(colliding_id_1)};
            swap_and_delete_single_element(colliding_id_1);

            const auto element_id = tree_handle.element();
            auto&& m_tree = layout_.modifyable_segment_tree(element_id);

            auto first_index = m_tree.add_tree(tree_copy);
            for (auto segment_index : range(first_index, ++m_tree.last_index())) {
                cache_insert(element_id, segment_index);
            }
        }
    }

    if (!tree_handle) {
        // create new empty tree
        const auto element_id = layout_.add_line_tree(SegmentTree {});
        {
            const auto element = schematic_.add_element({
                .element_type = ElementType::wire,
                .input_count = 0,
                .output_count = 0,
            });
            if (element.element_id() != element_id) [[unlikely]] {
                throw_exception("Added element ids don't match.");
            }
        }
        key_insert(element_id);
        tree_handle.set_element(element_id);
    }

    {
        // insert new segment with dummy endpoints
        const auto element_id = tree_handle.element();
        auto&& m_tree = layout_.modifyable_segment_tree(element_id);

        const auto segment = segment_info_t {
            .line = line,
            .p0_type = SegmentPointType::shadow_point,
            .p1_type = SegmentPointType::shadow_point,
        };

        const auto segment_index = m_tree.add_segment(segment, display_state_t::normal);
        cache_insert(element_id, segment_index);

        if (selection != nullptr) {
            const auto segment_part = get_segment_part(segment.line);
            selection->add_segment(segment_t {element_id, segment_index}, segment_part);
        }
    }

    // now fix all endpoints at given positions
    fix_line_segments(line.p0);
    fix_line_segments(line.p1);

    // TODO insertion mode change

#ifndef NDEBUG
    layout_.segment_tree(tree_handle.element()).validate();
#endif
}

auto EditableCircuit::add_line_segment(point_t p0, point_t p1,
                                       LineSegmentType segment_type,
                                       InsertionMode insertion_mode)
    -> selection_handle_t {
    auto selection_handle = create_selection();

    // TODO what with p0 == p1

    switch (segment_type) {
        using enum LineSegmentType;

        case horizontal_first: {
            const auto pm = point_t {p1.x, p0.y};
            if (p0.x != pm.x) {
                add_line_segment(line_t {p0, pm}, insertion_mode, selection_handle.get());
            }
            if (pm.y != p1.y) {
                add_line_segment(line_t {pm, p1}, insertion_mode, selection_handle.get());
            }
            break;
        }
        case vertical_first: {
            const auto pm = point_t {p0.x, p1.y};
            if (p0.y != pm.y) {
                add_line_segment(line_t {p0, pm}, insertion_mode, selection_handle.get());
            }
            if (pm.x != p1.x) {
                add_line_segment(line_t {pm, p1}, insertion_mode, selection_handle.get());
            }
            break;
        }
    }

    return selection_handle;
}

auto EditableCircuit::is_position_valid(element_id_t element_id, int x, int y) const
    -> bool {
    if (!is_representable(x, y)) {
        return false;
    }
    const auto position = point_t {grid_t {x}, grid_t {y}};

    auto data = to_layout_calculation_data(schematic_, layout_, element_id);
    data.position = position;

    return is_representable_(data);
}

auto EditableCircuit::move_or_delete_element(element_id_t& element_id, int x, int y)
    -> bool {
    // only temporary items can be freely moved
    if (layout_.display_state(element_id) != display_state_t::new_temporary)
        [[unlikely]] {
        throw_exception("Only temporary items can be freely moded.");
    }

    if (!is_position_valid(element_id, x, y)) {
        change_insertion_mode(element_id, InsertionMode::temporary);
        swap_and_delete_single_element(element_id);
        return false;
    }

    const auto position = point_t {grid_t {x}, grid_t {y}};
    layout_.set_position(element_id, position);
    return true;
}

namespace {

auto position_calculator(const Layout& layout, int delta_x, int delta_y) {
    return [delta_x, delta_y, &layout](element_id_t element_id) {
        const auto& element_position = layout.position(element_id);

        const int x = element_position.x.value + delta_x;
        const int y = element_position.y.value + delta_y;

        return std::make_pair(x, y);
    };
};

}  // namespace

auto EditableCircuit::are_positions_valid(const Selection& selection, int delta_x,
                                          int delta_y) const -> bool {
    const auto get_position = position_calculator(layout_, delta_x, delta_y);

    const auto is_valid = [&](element_id_t element_id) {
        const auto [x, y] = get_position(element_id);
        return is_position_valid(element_id, x, y);
    };
    return std::ranges::all_of(selection.selected_elements(), is_valid);
}

auto EditableCircuit::move_or_delete_elements(selection_handle_t handle, int delta_x,
                                              int delta_y) -> void {
    if (!handle) {
        return;
    }
    const auto get_position = position_calculator(layout_, delta_x, delta_y);

    // TODO refactor to algorithm
    while (handle->selected_elements().size() > 0) {
        auto element_id = *handle->selected_elements().begin();
        handle->remove_element(element_id);

        const auto [x, y] = get_position(element_id);
        move_or_delete_element(element_id, x, y);
    }
}

auto EditableCircuit::change_insertion_mode(selection_handle_t handle,
                                            InsertionMode new_insertion_mode) -> void {
    if (!handle) {
        return;
    }

    // TODO refactor to algorithm
    while (handle->selected_elements().size() > 0) {
        auto element_id = *handle->selected_elements().begin();
        handle->remove_element(element_id);

        change_insertion_mode(element_id, new_insertion_mode);
    }
}

auto EditableCircuit::delete_all(selection_handle_t handle) -> void {
    if (!handle) {
        return;
    }

    // TODO refactor to algorithm
    while (handle->selected_elements().size() > 0) {
        auto element_id = *handle->selected_elements().begin();
        handle->remove_element(element_id);

        change_insertion_mode(element_id, InsertionMode::temporary);
        if (!element_id) {
            throw_exception("element disappeared in mode change.");
        }
        swap_and_delete_single_element(element_id);
    }
}

auto EditableCircuit::change_insertion_mode(element_id_t& element_id,
                                            InsertionMode new_insertion_mode) -> bool {
    if (schematic_.element(element_id).is_placeholder()) [[unlikely]] {
        throw_exception(
            "cannot change insertion mode of "
            "placeholders.");
    }
    const auto old_insertion_mode = to_insertion_mode(layout_.display_state(element_id));

    if (old_insertion_mode == new_insertion_mode) {
        return true;
    }

    // transition: temporary -> collisions
    if (old_insertion_mode == InsertionMode::temporary) {
        // check collisions
        const auto data = to_layout_calculation_data(schematic_, layout_, element_id);
        const bool colliding = is_colliding(data);

        const auto display_state
            = colliding ? display_state_t::new_colliding : display_state_t::new_valid;
        layout_.set_display_state(element_id, display_state);

        if (!colliding) {
            // add to cache
            connect_and_cache_element(element_id);
        }

        if (new_insertion_mode == InsertionMode::collisions) {
            return true;
        }
    }

    // transition: collisions -> insert or discard
    if (new_insertion_mode == InsertionMode::insert_or_discard) {
        const auto display_state = layout_.display_state(element_id);

        if (display_state == display_state_t::new_valid) {
            layout_.set_display_state(element_id, display_state_t::normal);
            return true;
        }
        if (display_state == display_state_t::new_colliding) {
            // delete element
            layout_.set_display_state(element_id, display_state_t::new_temporary);
            swap_and_delete_single_element(element_id);
            return false;
        }

        throw_exception("unexpected display state at this point.");
    }

    // transition: insert or discard -> collisions
    if (old_insertion_mode == InsertionMode::insert_or_discard) {
        layout_.set_display_state(element_id, display_state_t::new_valid);

        if (new_insertion_mode == InsertionMode::collisions) {
            return true;
        }
    }

    // transition: collisions -> temporary
    if (new_insertion_mode == InsertionMode::temporary) {
        const auto display_state = layout_.display_state(element_id);

        if (display_state == display_state_t::new_valid) {
            // remove from cache
            cache_remove(element_id);
            layout_.set_display_state(element_id, display_state_t::new_temporary);

            disconnect_inputs_and_add_placeholders(element_id);
            disconnect_outputs_and_remove_placeholders(element_id);
            return true;
        }
        if (display_state == display_state_t::new_colliding) {
            layout_.set_display_state(element_id, display_state_t::new_temporary);
            return true;
        }

        throw_exception("unexpected display state at this point.");
    }

    throw_exception("unknown mode change");
}

auto EditableCircuit::selection_builder() const noexcept -> const SelectionBuilder& {
    return selection_builder_;
}

auto EditableCircuit::selection_builder() noexcept -> SelectionBuilder& {
    return selection_builder_;
}

auto EditableCircuit::query_selection(rect_fine_t rect) const
    -> std::vector<SearchTree::query_result_t> {
    return spatial_cache_.query_selection(rect);
};

auto EditableCircuit::query_selection(point_fine_t point) const
    -> std::optional<element_id_t> {
    auto query_result = spatial_cache_.query_selection(rect_fine_t {point, point});

    // TODO rethink this, maybe use find?
    auto elements = std::vector<element_id_t> {};
    transform_if(
        query_result, std::back_inserter(elements),
        [](const SearchTree::query_result_t& value) { return value.element_id; },
        [](const SearchTree::query_result_t& value) {
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

auto EditableCircuit::extract_schematic() -> Schematic {
    // TODO cleanup state
    return std::move(schematic_);
}

auto EditableCircuit::extract_layout() -> Layout {
    // TODO cleanup state
    return std::move(layout_);
}

auto EditableCircuit::swap_and_delete_multiple_elements(
    std::span<const element_id_t> element_ids) -> void {
    // sort descending, so we don't invalidate our ids
    auto sorted_ids = delete_queue_t {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (auto element_id : sorted_ids) {
        swap_and_delete_single_element(element_id);
    }
}

auto EditableCircuit::swap_and_delete_single_element(element_id_t& element_id) -> void {
    if (schematic_.element(element_id).element_type() == ElementType::wire) {
        // TODO merge this logic into change_insertion_mode
        for (auto&& index : range(layout_.segment_tree(element_id).segment_count())) {
            const auto segment_index
                = segment_index_t {gsl::narrow<segment_index_t::value_type>(index)};
            cache_remove(element_id, segment_index);
        }
        key_remove(element_id);
    } else {
        // TODO handle placeholders differently?
        if (!schematic_.element(element_id).is_placeholder()
            && layout_.display_state(element_id) != display_state_t::new_temporary)
            [[unlikely]] {
            throw_exception("can only delete temporary objects");
        }
        // change_insertion_mode(element_id, InsertionMode::temporary);
        key_remove(element_id);
    }

    // delete in underlying
    auto last_id1 = schematic_.swap_and_delete_element(element_id);
    auto last_id2 = layout_.swap_and_delete_element(element_id);
    if (last_id1 != last_id2) {
        throw_exception(
            "Returned id's during deletion are not the "
            "same.");
    }

    // update ids
    if (element_id != last_id1) {
        key_update(element_id, last_id1);
        cache_update(element_id, last_id1);
    }

    element_id = null_element;
}

auto EditableCircuit::add_and_connect_placeholder(Schematic::Output output)
    -> element_id_t {
    const auto placeholder_id = add_placeholder_element();
    const auto input = schematic_.element(placeholder_id).input(connection_id_t {0});
    output.connect(input);

    return placeholder_id;
}

auto EditableCircuit::disconnect_inputs_and_add_placeholders(element_id_t element_id)
    -> void {
    if (schematic_.element(element_id).is_placeholder()) {
        return;
    }

    for (const auto input : schematic_.element(element_id).inputs()) {
        if (input.has_connected_element()) {
            add_and_connect_placeholder(input.connected_output());
        }
    }
}

auto EditableCircuit::disconnect_outputs_and_remove_placeholders(element_id_t& element_id)
    -> void {
    auto delete_queue = delete_queue_t {};

    for (auto output : schematic_.element(element_id).outputs()) {
        if (output.has_connected_element()) {
            const auto connected_element = output.connected_element();

            if (connected_element.is_placeholder()) {
                delete_queue.push_back(connected_element.element_id());
            }
            output.clear_connection();
        }
    }

    const auto handle = element_handle(element_id);
    swap_and_delete_multiple_elements(delete_queue);
    element_id = handle.element();
}

auto EditableCircuit::add_missing_placeholders_for_outputs(element_id_t element_id)
    -> void {
    for (const auto output : schematic_.element(element_id).outputs()) {
        if (!output.has_connected_element()) {
            add_and_connect_placeholder(output);
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
                throw_exception(
                    "Connection is already connected at "
                    "this location.");
            }
            // mark placeholder for deletion
            unused_placeholder_id = found_connection.connected_element_id();
        }
        if (!orientations_compatible(connector.orientation, found_orientation)) {
            throw_exception(
                "Connection have incompatible "
                "orientations.");
        }

        // make connection in schematic
        connection.connect(found_connection);
    }

    return unused_placeholder_id;
}

auto EditableCircuit::connect_and_cache_element(element_id_t& element_id) -> void {
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
            // connect the input using the
            // output_connections cache
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
        // connect the output using the
        // input_connections cache
        const auto placeholder_id
            = connect_connector(output, input_connections_, schematic_);
        add_if_valid(placeholder_id);
        return true;
    });

    cache_insert(element_id);
    add_missing_placeholders_for_outputs(element_id);

    // this invalidates our element_id
    auto handle = element_handle(element_id);
    swap_and_delete_multiple_elements(delete_queue);
    element_id = handle.element();
}

// collisions

auto EditableCircuit::is_representable_(layout_calculation_data_t data) const -> bool {
    if (is_placeholder(data)) {
        return true;
    }
    if (data.element_type == ElementType::wire) {
        throw_exception("Not implemented for wires.");
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
    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("Not supported for wires.");
    }

    return collision_cache_.is_colliding(data) || input_connections_.is_colliding(data)
           || output_connections_.is_colliding(data);
}

auto EditableCircuit::is_colliding(line_t line) const -> bool {
    // TODO connections colliding

    return collision_cache_.is_colliding(line);
}

// keys

auto EditableCircuit::key_insert(element_id_t element_id) -> void {
    if (schematic_.element(element_id).is_placeholder()) {
        return;
    }
    selection_builder_.clear_cache();
}

auto EditableCircuit::key_remove(element_id_t element_id) -> void {
    const auto type = schematic_.element(element_id).element_type();

    if (type == ElementType::placeholder) {
        return;
    }
    selection_builder_.clear_cache();

    // elements
    for (auto&& entry : managed_selections_) {
        auto& selection = *entry.second;
        selection.remove_element(element_id);
    }

    // segments
    if (type == ElementType::wire) {
        const auto indices = layout_.segment_tree(element_id).indices();

        for (auto&& entry : managed_selections_) {
            auto& selection = *entry.second;

            for (auto&& segment_index : indices) {
                const auto segment = segment_t {element_id, segment_index};
                selection.remove_segment(segment);
            }
        }
    }
}

auto EditableCircuit::key_update(element_id_t new_element_id, element_id_t old_element_id)
    -> void {
    const auto type = schematic_.element(new_element_id).element_type();

    if (type == ElementType::placeholder) {
        return;
    }
    selection_builder_.clear_cache();

    // elements
    for (auto&& entry : managed_selections_) {
        auto& selection = *entry.second;
        selection.update_element_id(new_element_id, old_element_id);
    }

    // segments
    if (type == ElementType::wire) {
        const auto indices = layout_.segment_tree(new_element_id).indices();

        for (auto&& entry : managed_selections_) {
            auto& selection = *entry.second;

            for (auto&& segment_index : indices) {
                const auto new_segment = segment_t {new_element_id, segment_index};
                const auto old_segment = segment_t {old_element_id, segment_index};
                selection.update_segment_id(new_segment, old_segment);
            }
        }
    }
}

// caches

auto EditableCircuit::is_element_cached(element_id_t element_id) const -> bool {
    const auto display_state = layout_.display_state(element_id);
    return is_inserted(display_state);
}

auto EditableCircuit::cache_insert(element_id_t element_id) -> void {
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

auto EditableCircuit::cache_remove(element_id_t element_id) -> void {
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

auto EditableCircuit::cache_update(element_id_t new_element_id,
                                   element_id_t old_element_id) -> void {
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

auto EditableCircuit::cache_insert(element_id_t element_id, segment_index_t segment_index)
    -> void {
    const auto segment = layout_.segment_tree(element_id).segment(segment_index);

    input_connections_.insert(element_id, segment);
    output_connections_.insert(element_id, segment);
    collision_cache_.insert(element_id, segment);
    spatial_cache_.insert(element_id, segment.line, segment_index);
}

auto EditableCircuit::cache_remove(element_id_t element_id, segment_index_t segment_index)
    -> void {
    const auto segment = layout_.segment_tree(element_id).segment(segment_index);

    input_connections_.remove(element_id, segment);
    output_connections_.remove(element_id, segment);
    collision_cache_.remove(element_id, segment);
    spatial_cache_.remove(element_id, segment.line, segment_index);
}

// selections

auto EditableCircuit::create_selection() const -> selection_handle_t {
    const auto key = next_selection_key_++;

    auto&& [it, inserted]
        = managed_selections_.emplace(key, std::make_unique<Selection>());

    if (!inserted) {
        throw_exception("unable to create new selection.");
    }

    Selection& selection = *(it->second.get());
    return selection_handle_t {selection, *this, key};
}

auto EditableCircuit::create_selection(const Selection& selection) const
    -> selection_handle_t {
    auto handle = create_selection();
    handle.value() = selection;
    return handle;
}

auto EditableCircuit::element_handle() const -> element_handle_t {
    return element_handle_t {create_selection()};
}

auto EditableCircuit::element_handle(element_id_t element_id) const -> element_handle_t {
    auto handle = element_handle_t {create_selection()};
    handle.set_element(element_id);
    return handle;
}

auto EditableCircuit::delete_selection(selection_key_t selection_key) const -> void {
    if (!managed_selections_.erase(selection_key)) {
        throw_exception("unable to delete selection that should be present.");
    }
}

}  // namespace logicsim
