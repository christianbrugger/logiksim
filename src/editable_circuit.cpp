#include "editable_circuit.h"

#include "algorithm.h"
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

auto is_connection(SegmentPointType point_type) -> bool;

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

            case shadow_point:
            case cross_point:
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

//
// ElementKeyStore
//

auto ElementKeyStore::insert(element_id_t element_id) -> element_key_t {
    const auto element_key = next_key_;
    ++next_key_.value;

    insert(element_id, element_key);

    return element_key;
}

auto ElementKeyStore::insert(element_id_t element_id, element_key_t element_key) -> void {
    if (bool was_inserted = map_to_key_.insert({element_id, element_key}).second;
        !was_inserted) [[unlikely]] {
        throw_exception("Element id already exists in key store.");
    }

    if (bool was_inserted = map_to_id_.insert({element_key, element_id}).second;
        !was_inserted) [[unlikely]] {
        throw_exception("Element key already exists in key store.");
    }
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
    const auto element_key = to_element_key(old_element_id);
    remove(old_element_id);
    insert(new_element_id, element_key);
}

auto ElementKeyStore::element_key_valid(element_key_t element_key) const -> bool {
    if (element_key < element_key_t {0} || element_key >= next_key_) [[unlikely]] {
        throw_exception("This key was never handed out.");
    }

    return map_to_id_.find(element_key) != map_to_id_.end();
}

auto ElementKeyStore::to_element_id(element_key_t element_key) const -> element_id_t {
    const auto it = map_to_id_.find(element_key);

    if (it == map_to_id_.end()) [[unlikely]] {
        throw_exception("Element key not found in key store.");
    }

    return it->second;
}

auto ElementKeyStore::to_element_key(element_id_t element_id) const -> element_key_t {
    const auto it = map_to_key_.find(element_id);

    if (it == map_to_key_.end()) [[unlikely]] {
        throw_exception("Element id not found in key store.");
    }

    return it->second;
}

auto ElementKeyStore::to_element_ids(std::span<const element_key_t> element_keys) const
    -> std::vector<element_id_t> {
    return transform_to_vector(element_keys, [&](element_key_t element_key) {
        return to_element_id(element_key);
    });
}

auto ElementKeyStore::to_element_keys(std::span<const element_id_t> element_ids) const
    -> std::vector<element_key_t> {
    return transform_to_vector(
        element_ids, [&](element_id_t element_id) { return to_element_key(element_id); });
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
    : selection_builder_ {*this},
      schematic_ {std::move(schematic)},
      layout_ {std::move(layout)} {}

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
    schematic_.validate(Schematic::validate_all);

    // TODO validate layout
    // TODO validate caches, etc.
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
        throw_exception(
            "Input count needs to be at least 2 for "
            "standard elements.");
    }

    // auto selection_handle = create_selection();

    // insert into underlyings
    const auto element_id = layout_.add_logic_element(point_t {0, 0}, orientation,
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
    const auto element_key = key_insert(element_id);

    // validates our position
    if (!move_or_delete_element(element_key, position)) {
        return null_element_key;
    }
    if (!change_insertion_mode(element_key, insertion_mode)) {
        return null_element_key;
    }
    return element_key;
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
    const auto line0 = segment_info_0.line;
    const auto line1 = segment_info_1.line;

    if (line0.p0 == line1.p0) {
        return segment_info_t {
            .line = line_t {line0.p1, line1.p1},
            .p0_type = segment_info_0.p1_type,
            .p1_type = segment_info_1.p1_type,
        };
    }

    if (line0.p0 == line1.p1) {
        return segment_info_t {
            .line = line_t {line0.p1, line1.p0},
            .p0_type = segment_info_0.p1_type,
            .p1_type = segment_info_1.p0_type,
        };
    }

    if (line0.p1 == line1.p0) {
        return segment_info_t {
            .line = line_t {line0.p0, line1.p1},
            .p0_type = segment_info_0.p0_type,
            .p1_type = segment_info_1.p1_type,
        };
    }

    if (line0.p1 == line1.p1) {
        return segment_info_t {
            .line = line_t {line0.p0, line1.p0},
            .p0_type = segment_info_0.p0_type,
            .p1_type = segment_info_1.p0_type,
        };
    }

    throw_exception("segments need to have commont on shared point");
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

    if (!is_collision_considered(m_tree.display_state(index0))
        || !is_collision_considered(m_tree.display_state(index1))) [[unlikely]] {
        throw_exception("Can only merge collision considered segments.");
    }

    // merged segment
    const auto merged_segment
        = merge_parallel_segments(m_tree.segment(index0), m_tree.segment(index1));

    // TODO depends on mode
    // remove from cache
    cache_remove(element_id, index0);
    cache_remove(element_id, index1);
    if (index1 != last_index) {
        cache_remove(element_id, last_index);
    }

    // merge
    m_tree.update_segment(index0, merged_segment, display_state_t::normal);
    m_tree.swap_and_delete_segment(index1);

    // TODO depends on mode
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
               || is_collision_considered(tree.display_state(value.segment_index));
    });
}

auto EditableCircuit::fix_line_segments(point_t position) -> void {
    // TODO rename to segments
    const auto segment = selection_cache_.query_line_segments(position);
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
            set_segment_point_types(
                {
                    std::pair {lines.at(1).second, SegmentPointType::cross_point},
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
            set_segment_point_types(
                {
                    std::pair {lines.at(1).second, SegmentPointType::shadow_point},
                    std::pair {lines.at(2).second, SegmentPointType::cross_point},
                },
                position);
        } else {
            set_segment_point_types(
                {
                    std::pair {segment.at(0), SegmentPointType::colliding_point},
                    std::pair {segment.at(1), SegmentPointType::shadow_point},
                    std::pair {segment.at(2), SegmentPointType::cross_point},
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
                std::pair {segment.at(3), SegmentPointType::cross_point},
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

    const auto colliding_id_0 = collicions_cache_.get_first_wire(line.p0);
    const auto colliding_id_1 = collicions_cache_.get_first_wire(line.p1);

    auto tree_key = null_element_key;

    if (colliding_id_0) {
        tree_key = to_element_key(colliding_id_0);
    }

    if (colliding_id_1) {
        if (!tree_key) {
            tree_key = to_element_key(colliding_id_1);
        } else {
            // merge two trees
            const auto tree_copy = SegmentTree {layout_.segment_tree(colliding_id_1)};
            swap_and_delete_single_element(colliding_id_1);

            const auto element_id = to_element_id(tree_key);
            auto&& m_tree = layout_.modifyable_segment_tree(element_id);

            auto first_index = m_tree.add_tree(tree_copy);
            // TODO add only in specific mode?
            for (auto index : range(gsl::narrow<std::size_t>(first_index.value),
                                    m_tree.segment_count())) {
                const auto segment_index = segment_index_t {
                    gsl::narrow_cast<segment_index_t::value_type>(index)};
                cache_insert(element_id, segment_index);
            }
        }
    }

    if (tree_key == null_element_key) {
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
        tree_key = key_insert(element_id);
    }

    {
        // insert new segment with dummy endpoints
        const auto element_id = to_element_id(tree_key);
        auto&& m_tree = layout_.modifyable_segment_tree(element_id);

        const auto segment = segment_info_t {
            .line = line,
            .p0_type = SegmentPointType::shadow_point,
            .p1_type = SegmentPointType::shadow_point,
        };

        const auto segment_index = m_tree.add_segment(segment, display_state_t::normal);
        // TODO add only in specific mode?
        cache_insert(element_id, segment_index);

        if (selection != nullptr) {
            const auto segment_selection = get_segment_selection(segment.line);
            selection->add_segment(tree_key, segment_index, segment_selection);
        }
    }

    // now fix all endpoints at given positions
    fix_line_segments(line.p0);
    fix_line_segments(line.p1);

    // TODO output placeholders
    // TODO insertion mode change

#ifndef NDEBUG
    layout_.segment_tree(to_element_id(tree_key)).validate();
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

auto EditableCircuit::is_position_valid(element_key_t element_key, int x, int y) const
    -> bool {
    if (!is_representable(x, y)) {
        return false;
    }
    return is_position_valid(element_key, point_t {grid_t {x}, grid_t {y}});
}

auto EditableCircuit::is_position_valid(element_key_t element_key, point_t position) const
    -> bool {
    const auto element_id = to_element_id(element_key);

    auto data = to_layout_calculation_data(schematic_, layout_, element_id);
    data.position = position;

    return is_representable_(data);
}

auto EditableCircuit::move_or_delete_element(element_key_t element_key, point_t position)
    -> bool {
    if (!is_position_valid(element_key, position)) {
        delete_element(element_key);
        return false;
    }

    // only temporary items can be freely moved
    const auto element_id = to_element_id(element_key);
    const auto display_state = layout_.display_state(element_id);
    if (to_insertion_mode(display_state) != InsertionMode::temporary) [[unlikely]] {
        throw_exception("Only temporary items can be freely moded.");
    }

    layout_.set_position(element_id, position);
    return true;
}

auto EditableCircuit::change_insertion_mode(element_key_t element_key,
                                            InsertionMode new_insertion_mode) -> bool {
    auto element_id = to_element_id(element_key);
    return change_insertion_mode(element_id, new_insertion_mode);
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
            element_id = null_element;
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

/*
auto EditableCircuit::add_wire(LineTree&& line_tree) ->
element_key_t { const auto delays =
calculate_output_delays(line_tree); const auto max_delay =
std::ranges::max(delays); const auto output_count =
delays.size();

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
    auto element_id =
layout_.add_wire(std::move(line_tree));
    {
        const auto element = schematic_.add_element({
            .element_type = ElementType::wire,
            .input_count = 1,
            .output_count = output_count,
            .output_delays = delays,
            .history_length = max_delay,
        });
        if (element.element_id() != element_id)
[[unlikely]] { throw_exception("Added element ids don't
match.");
        }
    }
    const auto element_key = key_insert(element_id);

    // connect
    connect_and_cache_element(element_id);
    return element_key;
}
*/

auto EditableCircuit::delete_element(element_key_t element_key) -> void {
    const auto element_id = to_element_id(element_key);

    if (schematic_.element(element_id).is_placeholder()) {
        throw_exception("cannot directly delete placeholders.");
    }

    swap_and_delete_single_element(element_id);
}

auto EditableCircuit::selection_builder() const noexcept -> const SelectionBuilder& {
    return selection_builder_;
}

auto EditableCircuit::selection_builder() noexcept -> SelectionBuilder& {
    return selection_builder_;
}

auto EditableCircuit::to_element_id(element_key_t element_key) const -> element_id_t {
    return element_keys_.to_element_id(element_key);
}

auto EditableCircuit::to_element_ids(std::span<const element_key_t> element_keys) const
    -> std::vector<element_id_t> {
    return element_keys_.to_element_ids(element_keys);
}

auto EditableCircuit::to_element_key(element_id_t element_id) const -> element_key_t {
    return element_keys_.to_element_key(element_id);
}

auto EditableCircuit::to_element_keys(std::span<const element_id_t> element_ids) const
    -> std::vector<element_key_t> {
    return element_keys_.to_element_keys(element_ids);
}

auto EditableCircuit::element_key_valid(element_key_t element_key) const -> bool {
    return element_keys_.element_key_valid(element_key);
}

auto EditableCircuit::query_selection(rect_fine_t rect) const
    -> std::vector<SearchTree::query_result_t> {
    return selection_cache_.query_selection(rect);
};

auto EditableCircuit::query_selection(point_fine_t point) const
    -> std::optional<element_id_t> {
    auto query_result = selection_cache_.query_selection(rect_fine_t {point, point});

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
    if (schematic_.element(element_id).element_type() == ElementType::wire) {
        // TODO merge this logic into change_insertion_mode
        for (auto&& index : range(layout_.segment_tree(element_id).segment_count())) {
            const auto segment_index
                = segment_index_t {gsl::narrow<segment_index_t::value_type>(index)};
            cache_remove(element_id, segment_index);
        }
        key_remove(element_id);
    } else {
        if (!schematic_.element(element_id).is_placeholder()) {
            change_insertion_mode(element_id, InsertionMode::temporary);
            key_remove(element_id);
        }
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

    const auto element_key = to_element_key(element_id);
    swap_and_delete_multiple_elements(delete_queue);
    element_id = to_element_id(element_key);
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
    const auto element_key = to_element_key(element_id);
    swap_and_delete_multiple_elements(delete_queue);
    element_id = to_element_id(element_key);
}

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

    return collicions_cache_.is_colliding(data) || input_connections_.is_colliding(data)
           || output_connections_.is_colliding(data);
}

auto EditableCircuit::is_colliding(line_t line) const -> bool {
    // TODO connections colliding

    return collicions_cache_.is_colliding(line);
}

auto EditableCircuit::is_element_cached(element_id_t element_id) const -> bool {
    const auto display_state = layout_.display_state(element_id);
    return is_collision_considered(display_state);
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

    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("Not supported for wires.");
    }

    input_connections_.insert(element_id, data);
    output_connections_.insert(element_id, data);
    collicions_cache_.insert(element_id, data);
    selection_cache_.insert(element_id, data);
}

auto EditableCircuit::cache_remove(element_id_t element_id) -> void {
    const auto data = to_layout_calculation_data(schematic_, layout_, element_id);

    if (data.element_type == ElementType::wire) [[unlikely]] {
        throw_exception("Not supported for wires.");
    }

    input_connections_.remove(element_id, data);
    output_connections_.remove(element_id, data);
    collicions_cache_.remove(element_id, data);
    selection_cache_.remove(element_id, data);
}

auto EditableCircuit::cache_update(element_id_t new_element_id,
                                   element_id_t old_element_id) -> void {
    const auto element_type = schematic_.element(new_element_id).element_type();

    if (element_type == ElementType::placeholder) {
        return;
    }
    if (!is_element_cached(new_element_id)) {
        return;
    }
    // element cache update
    const auto data = to_layout_calculation_data(schematic_, layout_, new_element_id);

    // TODO never called for wires
    if (element_type != ElementType::wire) {
        // TODO connection cache
        input_connections_.update(new_element_id, old_element_id, data);
        output_connections_.update(new_element_id, old_element_id, data);
    }
    collicions_cache_.update(new_element_id, old_element_id, data);
    selection_cache_.update(new_element_id, old_element_id, data);
}

auto EditableCircuit::cache_insert(element_id_t element_id, segment_index_t segment_index)
    -> void {
    const auto segment = layout_.segment_tree(element_id).segment(segment_index);

    // TODO connection cache
    collicions_cache_.insert(element_id, segment);
    selection_cache_.insert(element_id, segment.line, segment_index);
}

auto EditableCircuit::cache_remove(element_id_t element_id, segment_index_t segment_index)
    -> void {
    const auto segment = layout_.segment_tree(element_id).segment(segment_index);

    // TODO connection cache
    collicions_cache_.remove(element_id, segment);
    selection_cache_.remove(element_id, segment.line, segment_index);
}

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

auto EditableCircuit::delete_selection(selection_key_t selection_key) const -> void {
    if (!managed_selections_.erase(selection_key)) {
        throw_exception("unable to delete selection that should be present.");
    }
}

}  // namespace logicsim
