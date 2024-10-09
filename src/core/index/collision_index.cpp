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

inline auto collision_point_t::format() const -> std::string {
    return fmt::format("<collision_point: {}, {}>", position, type);
}

}  // namespace

}  // namespace collision_index

//
// Implementation
//

namespace collision_index {

auto collision_data_t::format() const -> std::string {
    return fmt::format("<collision_data: {}, {}, {}, {}>", element_id_,
                       wire_id_horizontal_, wire_id_vertical_, to_state());
}

auto collision_data_t::empty() const -> bool {
    return element_id_ == null_logicitem_id && wire_id_horizontal_ == null_wire_id &&
           wire_id_vertical_ == null_wire_id;
}

auto collision_data_t::is_element_body() const -> bool {
    return element_id_                             //
           && wire_id_horizontal_ == null_wire_id  //
           && wire_id_vertical_ == null_wire_id;
}

auto collision_data_t::is_element_connection() const -> bool {
    return element_id_                             //
           && wire_id_horizontal_ == null_wire_id  //
           && wire_id_vertical_ == connection_tag;
}

auto collision_data_t::is_decoration() const -> bool {
    return false;
}

auto collision_data_t::is_wire_connection() const -> bool {
    return element_id_ == null_logicitem_id  //
           && wire_id_horizontal_            //
           && wire_id_vertical_ == connection_tag;
}

auto collision_data_t::is_wire_horizontal() const -> bool {
    return element_id_ == null_logicitem_id  //
           && wire_id_horizontal_            //
           && wire_id_vertical_ == null_wire_id;
}

auto collision_data_t::is_wire_vertical() const -> bool {
    return element_id_ == null_logicitem_id        //
           && wire_id_horizontal_ == null_wire_id  //
           && wire_id_vertical_;
}

auto collision_data_t::is_wire_corner_point() const -> bool {
    return element_id_ == wire_corner_point_tag  //
           && wire_id_horizontal_                //
           && wire_id_vertical_                  //
           && wire_id_horizontal_ == wire_id_vertical_;
}

auto collision_data_t::is_wire_cross_point() const -> bool {
    return element_id_ == wire_cross_point_tag  //
           && wire_id_horizontal_               //
           && wire_id_vertical_                 //
           && wire_id_horizontal_ == wire_id_vertical_;
}

// inferred states -> two elements

auto collision_data_t::is_wire_crossing() const -> bool {
    return element_id_ == null_logicitem_id  //
           && wire_id_horizontal_            //
           && wire_id_vertical_;
}

auto collision_data_t::is_element_wire_connection() const -> bool {
    return element_id_             //
           && wire_id_horizontal_  //
           && wire_id_vertical_ == connection_tag;
}

auto collision_data_t::to_state() const -> IndexState {
    using enum IndexState;

    if (is_element_body()) {
        return logicitem_body;
    }
    if (is_element_connection()) {
        return logicitem_connection;
    }
    if (is_wire_connection()) {
        return wire_connection;
    }
    if (is_wire_horizontal()) {
        return wire_horizontal;
    }
    if (is_wire_vertical()) {
        return wire_vertical;
    }
    if (is_wire_corner_point()) {
        return wire_corner_point;
    }
    if (is_wire_cross_point()) {
        return wire_cross_point;
    }

    // inferred states -> two elements
    if (is_wire_crossing()) {
        return wire_crossing;
    }
    if (is_element_wire_connection()) {
        return element_wire_connection;
    }

    if (empty()) {
        throw std::runtime_error("empty state");
    }

    // invalid state
    std::terminate();
}

auto collision_data_t::get_first_wire() const -> wire_id_t {
    if (wire_id_horizontal_) {
        return wire_id_horizontal_;
    }
    if (wire_id_vertical_) {
        return wire_id_vertical_;
    }
    return null_wire_id;
}

auto collision_data_t::set_connection_tag() -> void {
    if (wire_id_vertical_ != null_wire_id &&
        wire_id_vertical_ != collision_index::connection_tag) {
        throw std::runtime_error("cannot set connection tag, wire_id_vertical occupied");
    }
    wire_id_vertical_ = collision_index::connection_tag;
};

auto collision_data_t::set_wire_corner_point_tag() -> void {
    if (element_id_ != null_logicitem_id &&
        element_id_ != collision_index::wire_corner_point_tag) {
        throw std::runtime_error(
            "cannot set wire_corner_point tag, element body is occupied");
    }
    element_id_ = collision_index::wire_corner_point_tag;
};

auto collision_data_t::set_wire_cross_point_tag() -> void {
    if (element_id_ != null_logicitem_id &&
        element_id_ != collision_index::wire_cross_point_tag) {
        throw std::runtime_error(
            "cannot set wire_corner_point tag, element body is occupied");
    }
    element_id_ = collision_index::wire_cross_point_tag;
};

auto collision_data_t::set_logicitem_state(ItemType item_type,
                                           logicitem_id_t verify_old_id,
                                           logicitem_id_t set_new_id) -> void {
    const auto check_and_update = [&](logicitem_id_t& obj) {
        if (obj != verify_old_id) {
            throw std::runtime_error("unexpected collision state");
        }
        obj = set_new_id;
    };

    switch (item_type) {
        using enum collision_index::ItemType;

        case logicitem_body: {
            check_and_update(element_id_);
            break;
        }
        case logicitem_connection: {
            set_connection_tag();
            check_and_update(element_id_);
            break;
        }

        default: {
            throw std::runtime_error("Item type not a logic item");
        }
    }
}

auto collision_data_t::set_wire_state(ItemType item_type, wire_id_t verify_old_id,
                                      wire_id_t set_new_id) -> void {
    const auto check_and_update = [&](wire_id_t& obj) {
        if (obj != verify_old_id) {
            throw std::runtime_error("unexpected collision state");
        }
        obj = set_new_id;
    };

    switch (item_type) {
        using enum collision_index::ItemType;

        case wire_connection: {
            set_connection_tag();
            check_and_update(wire_id_horizontal_);
            break;
        }
        case wire_horizontal: {
            check_and_update(wire_id_horizontal_);
            break;
        }
        case wire_vertical: {
            check_and_update(wire_id_vertical_);
            break;
        }
        case wire_corner_point: {
            set_wire_corner_point_tag();
            check_and_update(wire_id_horizontal_);
            check_and_update(wire_id_vertical_);
            break;
        }
        case wire_cross_point: {
            set_wire_cross_point_tag();
            check_and_update(wire_id_horizontal_);
            check_and_update(wire_id_vertical_);
            break;
        }

        default: {
            throw std::runtime_error("Item type not a wire");
        }
    }
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
        case decoration:
            return "decoration_body";
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
        case decoration:
            return "decoration";
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

auto set_logicitem_state(CollisionIndex::map_type& map, point_t position,
                         collision_index::ItemType item_type,
                         logicitem_id_t verify_old_id,
                         logicitem_id_t set_new_id) -> void {
    // creates new entry, if it doesn't exist
    auto& data = map[position];
    data.set_logicitem_state(item_type, verify_old_id, set_new_id);

    if (data.empty()) {
        map.erase(position);
    }
}

auto set_wire_state(CollisionIndex::map_type& map, point_t position,
                    collision_index::ItemType item_type, wire_id_t verify_old_id,
                    wire_id_t set_new_id) -> void {
    // creates new entry, if it doesn't exist
    auto& data = map[position];
    data.set_wire_state(item_type, verify_old_id, set_new_id);

    if (data.empty()) {
        map.erase(position);
    }
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
                return !data.is_wire_connection();
            }
            case decoration: {
                return true;
            }
            case wire_connection: {
                return !data.is_element_connection();
            }
            case wire_horizontal: {
                return !data.is_wire_vertical();
            }
            case wire_vertical: {
                return !data.is_wire_horizontal();
            }
            case wire_corner_point: {
                return true;
            }
            case wire_cross_point: {
                return true;
            }
            case wire_new_unknown_point: {
                return data.is_element_body() || data.is_decoration() ||
                       data.is_element_wire_connection() || data.is_wire_crossing();
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
        return it->second.get_first_wire();
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

    return it->second.is_wire_crossing();
}

auto CollisionIndex::is_wire_cross_point(point_t point) const -> bool {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return false;
    }

    return it->second.is_wire_cross_point();
}

auto CollisionIndex::query(point_t point) const -> collision_index::collision_data_t {
    const auto it = map_.find(point);

    if (it == map_.end()) {
        return collision_index::collision_data_t {};
    }

    return it->second;
}

}  // namespace logicsim
