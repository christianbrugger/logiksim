#include "collision_index.h"

#include "algorithm/fmt_join.h"
#include "algorithm/range.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/line.h"
#include "geometry/orientation.h"
#include "index/collision_index.h"
#include "layout_info.h"
#include "layout_message.h"
#include "layout_message_generation.h"

#include <folly/small_vector.h>

#include <exception>
#include <stdexcept>

//
// Local Definitions
//

namespace logicsim {

namespace collision_index {

namespace {

struct collision_point_t {
    point_t position;
    ItemType type;

    auto operator==(const collision_point_t& other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<collision_point_t>);

constexpr inline auto collision_points_size =
    inputs_vector_size + outputs_vector_size + body_points_vector_size;
using collision_points_t = folly::small_vector<collision_point_t, collision_points_size>;

}  // namespace
}  // namespace collision_index

//
// Implementation
//

namespace collision_index {

auto is_element_body(collision_data_t data) -> bool {
    return data.logicitem_id_body                      //
           && data.wire_id_horizontal == null_wire_id  //
           && data.wire_id_vertical == null_wire_id;
}

auto is_element_connection(collision_data_t data) -> bool {
    return data.logicitem_id_body                      //
           && data.wire_id_horizontal == null_wire_id  //
           && data.wire_id_vertical == connection_tag;
}

auto is_wire_connection(collision_data_t data) -> bool {
    return data.logicitem_id_body == null_logicitem_id  //
           && data.wire_id_horizontal                   //
           && data.wire_id_vertical == connection_tag;
}

auto is_wire_horizontal(collision_data_t data) -> bool {
    return data.logicitem_id_body == null_logicitem_id  //
           && data.wire_id_horizontal                   //
           && data.wire_id_vertical == null_wire_id;
}

auto is_wire_vertical(collision_data_t data) -> bool {
    return data.logicitem_id_body == null_logicitem_id  //
           && data.wire_id_horizontal == null_wire_id   //
           && data.wire_id_vertical;
}

auto is_wire_corner_point(collision_data_t data) -> bool {
    return data.logicitem_id_body == wire_corner_point_tag  //
           && data.wire_id_horizontal                       //
           && data.wire_id_vertical                         //
           && data.wire_id_horizontal == data.wire_id_vertical;
}

auto is_wire_cross_point(collision_data_t data) -> bool {
    return data.logicitem_id_body == wire_cross_point_tag  //
           && data.wire_id_horizontal                      //
           && data.wire_id_vertical                        //
           && data.wire_id_horizontal == data.wire_id_vertical;
}

// inferred states -> two elements

auto is_wire_crossing(collision_data_t data) -> bool {
    return data.logicitem_id_body == null_logicitem_id  //
           && data.wire_id_horizontal                   //
           && data.wire_id_vertical;

    // && data.wire_id_horizontal != data.wire_id_vertical
    // && data.wire_id_horizontal == data.wire_id_vertical
}

auto is_element_wire_connection(collision_data_t data) -> bool {
    return data.logicitem_id_body      //
           && data.wire_id_horizontal  //
           && data.wire_id_vertical == connection_tag;
}

auto to_state(collision_data_t data) -> IndexState {
    using enum IndexState;

    if (is_element_body(data)) {
        return logicitem_body;
    }
    if (is_element_connection(data)) {
        return logicitem_connection;
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

inline auto collision_point_t::format() const -> std::string {
    return fmt::format("<collision_point: {}, {}>", position, type);
}

auto collision_data_t::format() const -> std::string {
    return fmt::format("<collision_data: {}, {}, {}, {}>", logicitem_id_body,
                       wire_id_horizontal, wire_id_vertical, to_state(*this));
}

}  // namespace collision_index

template <>
auto format(collision_index::ItemType type) -> std::string {
    switch (type) {
        using enum collision_index::ItemType;

        case logicitem_body:
            return "logicitem_body";
        case logicitem_connection:
            return "logicitem_connection";
        case wire_connection:
            return "wire_connection";
        case wire_horizontal:
            return "wire_horizontal";
        case wire_vertical:
            return "wire_vertical";
        case wire_corner_point:
            return "wire_corner_point";
        case wire_cross_point:
            return "wire_cross_point";

        case wire_new_unknown_point:
            return "wire_new_unknown_point";
    }
    std::terminate();
}

template <>
auto format(collision_index::IndexState state) -> std::string {
    switch (state) {
        using enum collision_index::IndexState;

        case logicitem_body:
            return "logicitem_body";
        case logicitem_connection:
            return "logicitem_connection";
        case wire_connection:
            return "wire_connection";
        case wire_horizontal:
            return "wire_horizontal";
        case wire_vertical:
            return "wire_vertical";
        case wire_corner_point:
            return "wire_corner_point";
        case wire_cross_point:
            return "wire_cross_point";

        case wire_crossing:
            return "wire_crossing";
        case element_wire_connection:
            return "element_wire_connection";

        case invalid_state:
            return "invalid_state";
    }
    std::terminate();
}

//
//
//

namespace collision_index {

namespace {

/**
 * @brief: Returns all collision points of the logic item.
 */
auto collision_points(const layout_calculation_data_t& data) -> collision_points_t {
    const auto& inputs = input_locations(data);
    const auto& outputs = output_locations(data);
    const auto& body_points = element_body_points(data);

    auto result = collision_points_t {};
    result.reserve(inputs.size() + outputs.size() + body_points.size());

    for (const auto& info : inputs) {
        result.push_back({info.position, ItemType::logicitem_connection});
    }
    for (const auto& info : outputs) {
        result.push_back({info.position, ItemType::logicitem_connection});
    }
    for (const point_t& position : body_points) {
        result.push_back({position, ItemType::logicitem_body});
    }

    return result;
}

/**
 * @brief: Returns collision item type of the endpoints.
 *
 * Note that not all endpoints have a collision type.
 */
auto collision_item_type(SegmentPointType type) -> std::optional<ItemType> {
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
    std::terminate();
};

/**
 * @brief: Adds collision points of the segment endpoints to the buffer.
 */
auto add_collision_end_points(collision_points_t& buffer,
                              segment_info_t segment) -> void {
    if (const auto p0_type = collision_item_type(segment.p0_type)) {
        buffer.push_back({segment.line.p0, *p0_type});
    }
    if (const auto p1_type = collision_item_type(segment.p1_type)) {
        buffer.push_back({segment.line.p1, *p1_type});
    }
}

/**
 * @brief: Returns collision points of the segment endpoints.
 */
auto collision_end_points(segment_info_t segment) -> collision_points_t {
    auto result = collision_points_t {};
    static_assert(collision_points_size >= 2);  // static capacity check

    add_collision_end_points(result, segment);
    return result;
}

/**
 * @brief: Returns all points of the line segment.
 */
auto collision_points(segment_info_t segment) -> collision_points_t {
    auto result = collision_points_t {};
    result.reserve(distance(segment.line) + std::size_t {1});

    const auto line = segment.line;

    if (is_horizontal(line)) {
        for (auto x : range(line.p0.x + grid_t {1}, line.p1.x)) {
            result.push_back({point_t {x, line.p0.y}, ItemType::wire_horizontal});
        }
    } else {
        for (auto y : range(line.p0.y + grid_t {1}, line.p1.y)) {
            result.push_back({point_t {line.p0.x, y}, ItemType::wire_vertical});
        }
    }

    add_collision_end_points(result, segment);

    return result;
}

auto delete_if_empty(CollisionIndex::map_type& map, point_t position,
                     const collision_index::collision_data_t& data) {
    if (!data.logicitem_id_body && !data.wire_id_horizontal && !data.wire_id_vertical) {
        map.erase(position);
    }
}

auto set_connection_tag(collision_index::collision_data_t& data) {
    if (data.wire_id_vertical != null_wire_id &&
        data.wire_id_vertical != collision_index::connection_tag) {
        throw std::runtime_error("cannot set connection tag, wire_id_vertical occupied");
    }
    data.wire_id_vertical = collision_index::connection_tag;
};

auto set_wire_corner_point_tag(collision_index::collision_data_t& data) {
    if (data.logicitem_id_body != null_logicitem_id &&
        data.logicitem_id_body != collision_index::wire_corner_point_tag) {
        throw std::runtime_error(
            "cannot set wire_corner_point tag, element body is occupied");
    }
    data.logicitem_id_body = collision_index::wire_corner_point_tag;
};

auto set_wire_cross_point_tag(collision_index::collision_data_t& data) {
    if (data.logicitem_id_body != null_logicitem_id &&
        data.logicitem_id_body != collision_index::wire_cross_point_tag) {
        throw std::runtime_error(
            "cannot set wire_corner_point tag, element body is occupied");
    }
    data.logicitem_id_body = collision_index::wire_cross_point_tag;
};

auto set_logicitem_state(CollisionIndex::map_type& map, point_t position,
                         collision_index::ItemType item_type,
                         logicitem_id_t verify_old_id,
                         logicitem_id_t set_new_id) -> void {
    const auto check_and_update = [&](logicitem_id_t& obj) {
        if (obj != verify_old_id) {
            throw std::runtime_error("unexpected collision state");
        }
        obj = set_new_id;
    };

    auto& data = map[position];

    switch (item_type) {
        using enum collision_index::ItemType;

        case logicitem_body: {
            check_and_update(data.logicitem_id_body);
            break;
        }
        case logicitem_connection: {
            set_connection_tag(data);
            check_and_update(data.logicitem_id_body);
            break;
        }

        default: {
            std::terminate();
        }
    }

    delete_if_empty(map, position, data);
}

auto set_wire_state(CollisionIndex::map_type& map, point_t position,
                    collision_index::ItemType item_type, wire_id_t verify_old_id,
                    wire_id_t set_new_id) -> void {
    const auto check_and_update = [&](wire_id_t& obj) {
        if (obj != verify_old_id) {
            throw std::runtime_error("unexpected collision state");
        }
        obj = set_new_id;
    };

    auto& data = map[position];

    switch (item_type) {
        using enum collision_index::ItemType;

        case wire_connection: {
            set_connection_tag(data);
            check_and_update(data.wire_id_horizontal);
            break;
        }
        case wire_horizontal: {
            check_and_update(data.wire_id_horizontal);
            break;
        }
        case wire_vertical: {
            check_and_update(data.wire_id_vertical);
            break;
        }
        case wire_corner_point: {
            set_wire_corner_point_tag(data);
            check_and_update(data.wire_id_horizontal);
            check_and_update(data.wire_id_vertical);
            break;
        }
        case wire_cross_point: {
            set_wire_cross_point_tag(data);
            check_and_update(data.wire_id_horizontal);
            check_and_update(data.wire_id_vertical);
            break;
        }

        default: {
            std::terminate();
        }
    }

    delete_if_empty(map, position, data);
}

}  // namespace

}  // namespace collision_index

CollisionIndex::CollisionIndex(const Layout& layout) {
    generate_inserted_layout_messages(*this, layout);
}

auto CollisionIndex::format() const -> std::string {
    if (map_.empty()) {
        return std::string {"CollisionIndex = []\n"};
    }

    return fmt::format("CollisionIndex ({} elements) = [\n  {}\n]\n", map_.size(),
                       fmt_join(",\n  ", map_));
}

auto CollisionIndex::allocated_size() const -> std::size_t {
    return get_allocated_size(map_);
}

auto CollisionIndex::handle(const info_message::LogicItemInserted& message) -> void {
    using namespace collision_index;

    for (const auto& item : collision_points(message.data)) {
        set_logicitem_state(map_, item.position, item.type, null_logicitem_id,
                            message.logicitem_id);
    }
}

auto CollisionIndex::handle(const info_message::InsertedLogicItemIdUpdated& message)
    -> void {
    using namespace collision_index;

    for (const auto& item : collision_points(message.data)) {
        set_logicitem_state(map_, item.position, item.type, message.old_logicitem_id,
                            message.new_logicitem_id);
    }
}

auto CollisionIndex::handle(const info_message::LogicItemUninserted& message) -> void {
    using namespace collision_index;

    for (const auto& item : collision_points(message.data)) {
        set_logicitem_state(map_, item.position, item.type, message.logicitem_id,
                            null_logicitem_id);
    }
}

auto CollisionIndex::handle(const info_message::SegmentInserted& message) -> void {
    using namespace collision_index;

    for (const auto& item : collision_points(message.segment_info)) {
        set_wire_state(map_, item.position, item.type, null_wire_id,
                       message.segment.wire_id);
    }
}

auto CollisionIndex::handle(const info_message::InsertedSegmentIdUpdated& message)
    -> void {
    using namespace collision_index;

    if (message.new_segment.wire_id == message.old_segment.wire_id) {
        return;
    }

    for (const auto& item : collision_points(message.segment_info)) {
        set_wire_state(map_, item.position, item.type, message.old_segment.wire_id,
                       message.new_segment.wire_id);
    }
}

auto CollisionIndex::handle(const info_message::InsertedEndPointsUpdated& message)
    -> void {
    using namespace collision_index;

    const auto wire_id = message.segment.wire_id;

    for (const auto& item : collision_end_points(message.old_segment_info)) {
        set_wire_state(map_, item.position, item.type, wire_id, null_wire_id);
    }
    for (const auto& item : collision_end_points(message.new_segment_info)) {
        set_wire_state(map_, item.position, item.type, null_wire_id, wire_id);
    }
}

auto CollisionIndex::handle(const info_message::SegmentUninserted& message) -> void {
    using namespace collision_index;

    for (const auto& item : collision_points(message.segment_info)) {
        set_wire_state(map_, item.position, item.type, message.segment.wire_id,
                       null_wire_id);
    }
}

auto CollisionIndex::submit(const InfoMessage& message) -> void {
    using namespace info_message;

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

auto CollisionIndex::state_colliding(point_t position,
                                     collision_index::ItemType item_type) const -> bool {
    if (const auto it = map_.find(position); it != map_.end()) {
        const auto data = it->second;

        switch (item_type) {
            using namespace collision_index;
            using enum collision_index::ItemType;

            case logicitem_body: {
                return true;
            }
            case logicitem_connection: {
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

auto CollisionIndex::is_colliding(const layout_calculation_data_t& data) const -> bool {
    using namespace collision_index;

    const auto is_colliding = [&](const collision_point_t& item) {
        return state_colliding(item.position, item.type);
    };

    return std::ranges::any_of(collision_points(data), is_colliding);
}

auto CollisionIndex::get_first_wire(point_t position) const -> wire_id_t {
    if (const auto it = map_.find(position); it != map_.end()) {
        const auto data = it->second;

        if (data.wire_id_horizontal) {
            return data.wire_id_horizontal;
        }
        if (data.wire_id_vertical) {
            return data.wire_id_vertical;
        }
    }
    return null_wire_id;
}

auto CollisionIndex::is_colliding(ordered_line_t line) const -> bool {
    using namespace collision_index;

    const auto segment = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::new_unknown,
        .p1_type = SegmentPointType::new_unknown,
    };

    const auto is_colliding = [&](const collision_point_t& item) {
        return state_colliding(item.position, item.type);
    };

    return std::ranges::any_of(collision_points(segment), is_colliding);
}

auto CollisionIndex::is_wires_crossing(point_t point) const -> bool {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return false;
    }

    return collision_index::is_wire_crossing(it->second);
}

auto CollisionIndex::is_wire_cross_point(point_t point) const -> bool {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return false;
    }

    return collision_index::is_wire_cross_point(it->second);
}

auto CollisionIndex::query(point_t point) const -> collision_index::collision_data_t {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return collision_index::collision_data_t {};
    }

    return it->second;
}

}  // namespace logicsim
