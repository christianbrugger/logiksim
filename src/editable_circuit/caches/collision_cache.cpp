#include "editable_circuit/caches/collision_cache.h"

#include "layout_calculations.h"

namespace logicsim {

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

auto CollisionCache::handle(editable_circuit::info_message::LogicItemInserted message)
    -> void {
    insert_impl(map_, message.element_id, message.data);
}

auto CollisionCache::handle(editable_circuit::info_message::LogicItemUninserted message)
    -> void {
    remove_impl(map_, message.element_id, message.data);
}

auto CollisionCache::handle(
    editable_circuit::info_message::InsertedLogicItemIdUpdated message) -> void {
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
    editable_circuit::info_message::InsertedSegmentIdUpdated message) -> void {
    update_impl(map_, message.new_segment.element_id, message.old_segment.element_id,
                message.segment_info);
}

auto CollisionCache::submit(editable_circuit::InfoMessage message) -> void {
    using namespace editable_circuit::info_message;

    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemUninserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedLogicItemIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<SegmentInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<SegmentUninserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedSegmentIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
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

auto CollisionCache::creates_loop(ordered_line_t line) const -> bool {
    const auto element_id_0 = get_first_wire(line.p0);
    const auto element_id_1 = get_first_wire(line.p1);

    return element_id_0 != null_element && element_id_0 == element_id_1;
}

auto CollisionCache::is_colliding(ordered_line_t line) const -> bool {
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

}  // namespace logicsim
