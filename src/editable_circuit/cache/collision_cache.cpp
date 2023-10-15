#include "editable_circuit/cache/collision_cache.h"

#include "algorithm/range.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/trait.h"
#include "editable_circuit/cache/helper.h"
#include "editable_circuit/message.h"
#include "exception.h"
#include "geometry/orientation.h"
#include "layout_info.h"

namespace logicsim {

namespace collision_cache {
auto is_element_body(collision_data_t data) -> bool {
    return data.element_id_body                           //
           && data.element_id_horizontal == null_element  //
           && data.element_id_vertical == null_element;
}

auto is_element_connection(collision_data_t data) -> bool {
    return data.element_id_body                           //
           && data.element_id_horizontal == null_element  //
           && data.element_id_vertical == connection_tag;
}

auto is_wire_connection(collision_data_t data) -> bool {
    return data.element_id_body == null_element  //
           && data.element_id_horizontal         //
           && data.element_id_vertical == connection_tag;
}

auto is_wire_horizontal(collision_data_t data) -> bool {
    return data.element_id_body == null_element  //
           && data.element_id_horizontal         //
           && data.element_id_vertical == null_element;
}

auto is_wire_vertical(collision_data_t data) -> bool {
    return data.element_id_body == null_element           //
           && data.element_id_horizontal == null_element  //
           && data.element_id_vertical;
}

auto is_wire_corner_point(collision_data_t data) -> bool {
    return data.element_id_body == wire_corner_point_tag  //
           && data.element_id_horizontal                  //
           && data.element_id_vertical                    //
           && data.element_id_horizontal == data.element_id_vertical;
}

auto is_wire_cross_point(collision_data_t data) -> bool {
    return data.element_id_body == wire_cross_point_tag  //
           && data.element_id_horizontal                 //
           && data.element_id_vertical                   //
           && data.element_id_horizontal == data.element_id_vertical;
}

// inferred states -> two elements

auto is_wire_crossing(collision_data_t data) -> bool {
    return data.element_id_body == null_element  //
           && data.element_id_horizontal         //
           && data.element_id_vertical;

    // && data.element_id_horizontal != data.element_id_vertical
    // && data.element_id_horizontal == data.element_id_vertical
}

auto is_element_wire_connection(collision_data_t data) -> bool {
    return data.element_id_body           //
           && data.element_id_horizontal  //
           && data.element_id_vertical == connection_tag;
}

auto to_state(collision_data_t data) -> CacheState {
    using enum CacheState;

    if (is_element_body(data)) {
        return element_body;
    }
    if (is_element_connection(data)) {
        return element_connection;
    }
    if (is_wire_connection(data)) {
        return wire_connection;
    }
    if (is_wire_horizontal(data)) {
        return wire_horizontal;
    }
    if (is_wire_vertical(data)) {
        return wire_vertical;
    }
    if (is_wire_corner_point(data)) {
        return wire_corner_point;
    }
    if (is_wire_cross_point(data)) {
        return wire_cross_point;
    }

    // inferred states -> two elements
    if (is_wire_crossing(data)) {
        return wire_crossing;
    }
    if (is_element_wire_connection(data)) {
        return element_wire_connection;
    }

    return invalid_state;
}

auto collision_data_t::format() const -> std::string {
    return fmt::format("<collision_data: {}, {}, {}>", element_id_body,
                       element_id_horizontal, element_id_vertical);
}

}  // namespace collision_cache

//
//
//

// next_state(point_t position, ItemType state) -> bool
template <typename Func>
auto iter_collision_state(const layout_calculation_data_t& data, Func next_state)
    -> bool {
    using collision_cache::ItemType;

    for (const auto& info : input_locations(data)) {
        if (!next_state(info.position, ItemType::element_connection)) {
            return false;
        }
    }

    for (const point_t& position : element_body_points(data)) {
        if (!next_state(position, ItemType::element_connection)) {
            return false;
        }
    }

    for (const auto& info : output_locations(data)) {
        if (!next_state(info.position, ItemType::element_connection)) {
            return false;
        }
    }

    return true;
}

// next_state(point_t position, ItemType state) -> bool
template <typename Func>
auto iter_collision_state_endpoints(segment_info_t segment, Func next_state) -> bool {
    using collision_cache::ItemType;

    const auto to_state = [](SegmentPointType type) -> std::optional<ItemType> {
        switch (type) {
            using enum SegmentPointType;

            case input:
            case output:
                return ItemType::wire_connection;
            case corner_point:
                return ItemType::wire_corner_point;
            case cross_point:
                return ItemType::wire_cross_point;

            case shadow_point:
                return std::nullopt;

            case new_unknown:
                return ItemType::wire_new_unknown_point;
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

// next_state(point_t position, ItemType state) -> bool
template <typename Func>
auto iter_collision_state(segment_info_t segment, Func next_state) -> bool {
    using enum collision_cache::ItemType;

    {
        const auto line = segment.line;

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

    return iter_collision_state_endpoints(segment, next_state);
}

namespace {
// apply_func(element_id_t& obj) -> void
template <typename Apply>
auto apply_function(CollisionCache::map_type& map, point_t position,
                    collision_cache::ItemType state, Apply apply_func) -> bool {
    auto& data = map[position];

    const auto set_connection_tag = [&]() {
        if (data.element_id_vertical) {
            throw_exception("cannot set connection tag, second element occupied");
        }
        data.element_id_vertical = collision_cache::connection_tag;
    };
    const auto set_wire_corner_point_tag = [&]() {
        if (data.element_id_body != null_element &&
            data.element_id_body != collision_cache::wire_corner_point_tag) {
            throw_exception("cannot set wire_corner_point tag, element body is occupied");
        }
        data.element_id_body = collision_cache::wire_corner_point_tag;
    };
    const auto set_wire_cross_point_tag = [&]() {
        if (data.element_id_body != null_element &&
            data.element_id_body != collision_cache::wire_cross_point_tag) {
            throw_exception("cannot set wire_corner_point tag, element body is occupied");
        }
        data.element_id_body = collision_cache::wire_cross_point_tag;
    };

    switch (state) {
        using enum collision_cache::ItemType;

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
        case wire_corner_point: {
            set_wire_corner_point_tag();
            apply_func(data.element_id_horizontal);
            apply_func(data.element_id_vertical);
            break;
        }
        case wire_cross_point: {
            set_wire_cross_point_tag();
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
    if (!data.element_id_body && !data.element_id_horizontal &&
        !data.element_id_vertical) {
        map.erase(position);
    }
    return true;
}
}  // namespace

auto get_check_empty_and_assign(element_id_t element_id) {
    return [element_id](element_id_t& obj) {
        if (obj != null_element) {
            throw_exception("collision state is not empty in insert.");
        }
        obj = element_id;
    };
}

auto get_check_and_delete(element_id_t element_id) {
    return [element_id](element_id_t& obj) {
        if (obj != element_id) {
            throw_exception("exected collision state presence in remove.");
        }
        obj = null_element;
    };
}

auto get_check_and_update(element_id_t new_element_id, element_id_t old_element_id) {
    return [new_element_id, old_element_id](element_id_t& obj) {
        if (obj != old_element_id) {
            throw_exception("exected collision state presence in update.");
        }
        obj = new_element_id;
    };
}

template <class Data>
auto insert_impl(CollisionCache::map_type& map, element_id_t element_id, Data data)
    -> void {
    const auto check_empty_and_assign = get_check_empty_and_assign(element_id);

    iter_collision_state(data, [&](point_t position, collision_cache::ItemType state) {
        return apply_function(map, position, state, check_empty_and_assign);
    });
}

template <class Data>
auto remove_impl(CollisionCache::map_type& map, element_id_t element_id, Data data)
    -> void {
    const auto check_and_delete = get_check_and_delete(element_id);

    iter_collision_state(data, [&](point_t position, collision_cache::ItemType state) {
        return apply_function(map, position, state, check_and_delete);
    });
}

template <class Data>
auto update_impl(CollisionCache::map_type& map, element_id_t new_element_id,
                 element_id_t old_element_id, Data data) -> void {
    const auto check_and_update = get_check_and_update(new_element_id, old_element_id);

    iter_collision_state(data, [&](point_t position, collision_cache::ItemType state) {
        return apply_function(map, position, state, check_and_update);
    });
}

auto CollisionCache::handle(
    const editable_circuit::info_message::InsertedEndPointsUpdated& message) -> void {
    const auto element_id = message.segment.element_id;
    const auto check_and_delete = get_check_and_delete(element_id);
    const auto check_empty_and_assign = get_check_empty_and_assign(element_id);

    iter_collision_state_endpoints(
        message.old_segment_info, [&](point_t position, collision_cache::ItemType state) {
            return apply_function(map_, position, state, check_and_delete);
        });
    iter_collision_state_endpoints(
        message.new_segment_info, [&](point_t position, collision_cache::ItemType state) {
            return apply_function(map_, position, state, check_empty_and_assign);
        });
}

auto CollisionCache::format() const -> std::string {
    // return fmt::format("CollisionCache = {}\n", map_);
    return "!!! NOT IMPLEMENTED !!!";
}

auto CollisionCache::allocated_size() const -> std::size_t {
    return get_allocated_size(map_);
}

auto CollisionCache::handle(
    const editable_circuit::info_message::LogicItemInserted& message) -> void {
    insert_impl(map_, message.element_id, message.data);
}

auto CollisionCache::handle(
    const editable_circuit::info_message::InsertedLogicItemIdUpdated& message) -> void {
    update_impl(map_, message.new_element_id, message.old_element_id, message.data);
}

auto CollisionCache::handle(
    const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    remove_impl(map_, message.element_id, message.data);
}

auto CollisionCache::handle(
    const editable_circuit::info_message::SegmentInserted& message) -> void {
    insert_impl(map_, message.segment.element_id, message.segment_info);
}

auto CollisionCache::handle(
    const editable_circuit::info_message::InsertedSegmentIdUpdated& message) -> void {
    if (message.new_segment.element_id == message.old_segment.element_id) {
        return;
    }
    update_impl(map_, message.new_segment.element_id, message.old_segment.element_id,
                message.segment_info);
}

auto CollisionCache::handle(
    const editable_circuit::info_message::SegmentUninserted& message) -> void {
    remove_impl(map_, message.segment.element_id, message.segment_info);
}

auto CollisionCache::submit(const editable_circuit::InfoMessage& message) -> void {
    using namespace editable_circuit::info_message;

    // logic items
    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedLogicItemIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemUninserted>(&message)) {
        handle(*pointer);
        return;
    }

    // segments
    if (auto pointer = std::get_if<SegmentInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedSegmentIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedEndPointsUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<SegmentUninserted>(&message)) {
        handle(*pointer);
        return;
    }
}

auto CollisionCache::state_colliding(point_t position,
                                     collision_cache::ItemType item_type) const -> bool {
    if (const auto it = map_.find(position); it != map_.end()) {
        const auto data = it->second;

        switch (item_type) {
            using namespace collision_cache;
            using enum collision_cache::ItemType;

            case element_body: {
                return true;
            }
            case element_connection: {
                return !is_wire_connection(data);
            }
            case wire_connection: {
                return !is_element_connection(data);
            }
            case wire_horizontal: {
                return !is_wire_vertical(data);
            }
            case wire_vertical: {
                return !is_wire_horizontal(data);
            }
            case wire_corner_point: {
                return true;
            }
            case wire_cross_point: {
                return true;
            }
            case wire_new_unknown_point: {
                return is_element_body(data) || is_element_wire_connection(data) ||
                       is_wire_crossing(data);
            }
        };
    }
    return false;
};

auto CollisionCache::is_colliding(const layout_calculation_data_t& data) const -> bool {
    const auto not_colliding = [&](point_t position, collision_cache::ItemType state) {
        return !state_colliding(position, state);
    };
    return !iter_collision_state(data, not_colliding);
}

auto CollisionCache::get_first_wire(point_t position) const -> element_id_t {
    if (const auto it = map_.find(position); it != map_.end()) {
        const auto data = it->second;

        if (data.element_id_horizontal) {
            return data.element_id_horizontal;
        }
        if (data.element_id_vertical) {
            return data.element_id_vertical;
        }
    }
    return null_element;
}

auto CollisionCache::is_colliding(ordered_line_t line) const -> bool {
    const auto segment = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::new_unknown,
        .p1_type = SegmentPointType::new_unknown,
    };

    const auto not_colliding = [&](point_t position, collision_cache::ItemType state) {
        return !state_colliding(position, state);
    };
    return !iter_collision_state(segment, not_colliding);
}

auto CollisionCache::is_wires_crossing(point_t point) const -> bool {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return false;
    }

    return collision_cache::is_wire_crossing(it->second);
}

auto CollisionCache::is_wire_cross_point(point_t point) const -> bool {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return false;
    }

    return collision_cache::is_wire_cross_point(it->second);
}

auto CollisionCache::query(point_t point) const -> collision_cache::collision_data_t {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return collision_cache::collision_data_t {};
    }

    return it->second;
}

auto CollisionCache::validate(const Layout& layout) const -> void {
    auto cache = CollisionCache {};
    add_layout_to_cache(cache, layout);

    if (cache.map_ != this->map_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

}  // namespace logicsim
