#include "editable_circuit/cache/connection_cache.h"

#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/trait.h"
#include "editable_circuit/cache/helper.h"
#include "editable_circuit/message.h"
#include "exception.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/orientation.h"
#include "layout_info.h"

#include <fmt/core.h>

namespace logicsim {

namespace connection_cache {

auto wire_value_t::format() const -> std::string {
    return fmt::format("Wire_{}-{}-{}", segment.wire_id, segment.segment_index,
                       orientation);
}

}  // namespace connection_cache

//
// ConnectionCache
//

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Input>::format() const
    -> std::string {
    return fmt::format("LogicItemInputCache = {}", map_);
}

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Output>::format() const
    -> std::string {
    return fmt::format("LogicItemOutputCache = {}", map_);
}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Input>::format() const
    -> std::string {
    return fmt::format("WireInputCache = {}", map_);
}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Output>::format() const
    -> std::string {
    return fmt::format("WireOutputCache = {}", map_);
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::allocated_size() const -> std::size_t {
    return get_allocated_size(map_);
}

namespace connection_cache {

namespace {

auto get_and_verify_cache_entry(map_type<ContentType::LogicItem>& map, point_t position,
                                logicitem_connection_t value)
    -> map_type<ContentType::LogicItem>::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw_exception("unable to find cached data that should be present.");
    }
    return it;
}

auto get_and_verify_cache_entry(map_type<ContentType::Wire>& map, point_t position,
                                wire_value_t value)
    -> map_type<ContentType::Wire>::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw_exception("unable to find cached data that should be present.");
    }
    return it;
}

auto verify_cache_empty(map_type<ContentType::LogicItem>& map, point_t position) -> void {
    if (map.contains(position)) [[unlikely]] {
        throw_exception("cache already has an entry at this position");
    }
}

auto verify_cache_empty(map_type<ContentType::Wire>& map, point_t position) -> void {
    if (map.contains(position)) [[unlikely]] {
        throw_exception("cache already has an entry at this position");
    }
}

}  // namespace

}  // namespace connection_cache

//
// LogicItemInserted
//

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemInserted& message) -> void {
    using namespace connection_cache;

    for (auto info : input_locations_and_id(message.data)) {
        verify_cache_empty(map_, info.position);

        map_[info.position] = logicitem_connection_t {
            message.logicitem_id,
            info.input_id,
            info.orientation,
        };
    }
}

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemInserted& message) -> void {
    using namespace connection_cache;

    for (auto info : output_locations_and_id(message.data)) {
        verify_cache_empty(map_, info.position);

        map_[info.position] = logicitem_connection_t {
            message.logicitem_id,
            info.output_id,
            info.orientation,
        };
    }
}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemInserted& message) -> void {}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemInserted& message) -> void {}

//
// InsertedLogicItemIdUpdated
//

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Input>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void {
    using namespace connection_cache;

    for (auto info : input_locations_and_id(message.data)) {
        const auto expected_value = logicitem_connection_t {
            message.old_logicitem_id,
            info.input_id,
            info.orientation,
        };

        const auto it = get_and_verify_cache_entry(map_, info.position, expected_value);
        it->second.logicitem_id = message.new_logicitem_id;
    }
}

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Output>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void {
    using namespace connection_cache;

    for (auto info : output_locations_and_id(message.data)) {
        const auto expected_value = logicitem_connection_t {
            message.old_logicitem_id,
            info.output_id,
            info.orientation,
        };

        const auto it = get_and_verify_cache_entry(map_, info.position, expected_value);
        it->second.logicitem_id = message.new_logicitem_id;
    }
}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Input>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void {}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Output>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void {}

//
// LogicItemUninserted
//

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    using namespace connection_cache;

    for (auto info : input_locations_and_id(message.data)) {
        const auto expected_value = logicitem_connection_t {
            message.logicitem_id,
            info.input_id,
            info.orientation,
        };

        const auto it = get_and_verify_cache_entry(map_, info.position, expected_value);
        map_.erase(it);
    }
}

template <>
auto ConnectionCache<connection_cache::ContentType::LogicItem,
                     connection_cache::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    using namespace connection_cache;

    for (auto info : output_locations_and_id(message.data)) {
        const auto expected_value = logicitem_connection_t {
            message.logicitem_id,
            info.output_id,
            info.orientation,
        };

        const auto it = get_and_verify_cache_entry(map_, info.position, expected_value);
        map_.erase(it);
    }
}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemUninserted& message) -> void {}

template <>
auto ConnectionCache<connection_cache::ContentType::Wire,
                     connection_cache::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemUninserted& message) -> void {}

//
// SegmentInserted
//

namespace connection_cache {
namespace {

using namespace editable_circuit::info_message;

template <connection_cache::DirectionType Direction>
consteval auto point_type() -> SegmentPointType {
    return Direction == DirectionType::Input ? SegmentPointType::input
                                             : SegmentPointType::output;
}

auto handle_wire(wire_map_t& map, SegmentPointType point_type,
                 const SegmentInserted& message) -> void {
    if (message.segment_info.p0_type == point_type) {
        const auto position = message.segment_info.line.p0;
        verify_cache_empty(map, position);

        map[position] = wire_value_t {
            .segment = message.segment,
            .orientation = to_orientation_p0(message.segment_info.line),
        };
    }

    if (message.segment_info.p1_type == point_type) {
        const auto position = message.segment_info.line.p1;
        verify_cache_empty(map, position);

        map[position] = wire_value_t {
            .segment = message.segment,
            .orientation = to_orientation_p1(message.segment_info.line),
        };
    }
}

//
// InsertedSegmentIdUpdated
//

auto handle_wire(wire_map_t& map, SegmentPointType point_type,
                 const InsertedSegmentIdUpdated& message) -> void {
    if (message.new_segment == message.old_segment) {
        return;
    }

    if (message.segment_info.p0_type == point_type) {
        const auto position = message.segment_info.line.p0;
        const auto expected_value = wire_value_t {
            .segment = message.old_segment,
            .orientation = to_orientation_p0(message.segment_info.line),
        };

        const auto it = get_and_verify_cache_entry(map, position, expected_value);
        it->second.segment = message.new_segment;
    }

    if (message.segment_info.p1_type == point_type) {
        const auto position = message.segment_info.line.p1;
        const auto expected_value = wire_value_t {
            .segment = message.old_segment,
            .orientation = to_orientation_p1(message.segment_info.line),
        };

        const auto it = get_and_verify_cache_entry(map, position, expected_value);
        it->second.segment = message.new_segment;
    }
}

//
// SegmentUninserted
//
auto handle_wire(wire_map_t& map, SegmentPointType point_type,
                 const SegmentUninserted& message) -> void {
    if (message.segment_info.p0_type == point_type) {
        const auto position = message.segment_info.line.p0;
        const auto expected_value = wire_value_t {
            .segment = message.segment,
            .orientation = to_orientation_p0(message.segment_info.line),
        };

        const auto it = get_and_verify_cache_entry(map, position, expected_value);
        map.erase(it);
    }

    if (message.segment_info.p1_type == point_type) {
        const auto position = message.segment_info.line.p1;
        const auto expected_value = wire_value_t {
            .segment = message.segment,
            .orientation = to_orientation_p1(message.segment_info.line),
        };

        const auto it = get_and_verify_cache_entry(map, position, expected_value);
        map.erase(it);
    }
}

}  // namespace
}  // namespace connection_cache

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::handle(
    const editable_circuit::info_message::SegmentInserted& message) -> void {
    if constexpr (Content == ContentType::Wire) {
        using namespace connection_cache;
        handle_wire(map_, point_type<Direction>(), message);
    }
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::handle(
    const editable_circuit::info_message::InsertedSegmentIdUpdated& message) -> void {
    if constexpr (Content == ContentType::Wire) {
        using namespace connection_cache;
        handle_wire(map_, point_type<Direction>(), message);
    }
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::handle(
    const editable_circuit::info_message::InsertedEndPointsUpdated& message) -> void {
    using namespace editable_circuit::info_message;

    handle(SegmentUninserted {message.segment, message.old_segment_info});
    handle(SegmentInserted {message.segment, message.new_segment_info});
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::handle(
    const editable_circuit::info_message::SegmentUninserted& message) -> void {
    if constexpr (Content == ContentType::Wire) {
        using namespace connection_cache;
        handle_wire(map_, point_type<Direction>(), message);
    }
}

//
// submit
//

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::submit(
    const editable_circuit::InfoMessage& message) -> void {
    using namespace editable_circuit::info_message;

    // logic items
    if constexpr (Content == ContentType::LogicItem) {
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
    }

    // wire segments
    if constexpr (Content == ContentType::Wire) {
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
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::find(point_t position) const
    -> std::optional<value_t> {
    if (const auto it = map_.find(position); it != map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::is_colliding(
    const layout_calculation_data_t& data) const -> bool {
    const auto same_type_not_colliding = [&](const auto& info) -> bool {
        return !map_.contains(info.position);
    };

    // make sure inputs match with output orientations, if present
    const auto different_type_compatible = [&](const auto& info) -> bool {
        if (const auto it = map_.find(info.position); it != map_.end()) {
            return orientations_compatible(info.orientation, it->second.orientation);
        }
        return true;
    };

    if constexpr (Direction == DirectionType::Input) {
        return !std::ranges::all_of(input_locations(data), same_type_not_colliding) ||
               !std::ranges::all_of(output_locations(data), different_type_compatible);

    } else {
        return !std::ranges::all_of(input_locations(data), different_type_compatible) ||
               !std::ranges::all_of(output_locations(data), same_type_not_colliding);
    }
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::is_colliding(point_t position,
                                                       orientation_t orientation) const
    -> bool {
    if (const auto it = map_.find(position); it != map_.end()) {
        return !orientations_compatible(orientation, it->second.orientation);
    }
    return false;
}

template <connection_cache::ContentType Content,
          connection_cache::DirectionType Direction>
auto ConnectionCache<Content, Direction>::validate(const Layout& layout) const -> void {
    auto cache = ConnectionCache<Content, Direction> {};
    add_layout_to_cache(cache, layout);

    if (cache.map_ != this->map_) [[unlikely]] {
        throw_exception("current cache state doesn't match circuit");
    }
}

template class ConnectionCache<connection_cache::ContentType::LogicItem,
                               connection_cache::DirectionType::Input>;
template class ConnectionCache<connection_cache::ContentType::LogicItem,
                               connection_cache::DirectionType::Output>;

template class ConnectionCache<connection_cache::ContentType::Wire,
                               connection_cache::DirectionType::Input>;
template class ConnectionCache<connection_cache::ContentType::Wire,
                               connection_cache::DirectionType::Output>;

}  // namespace logicsim
