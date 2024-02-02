#include "connection_index.h"

#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/trait.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/orientation.h"
#include "index/connection_index.h"
#include "layout_info.h"
#include "layout_message.h"
#include "layout_message_generation.h"

#include <fmt/core.h>

namespace logicsim {

namespace connection_index {

auto wire_value_t::format() const -> std::string {
    return fmt::format("Wire_{}-{}-{}", segment.wire_id, segment.segment_index,
                       orientation);
}

}  // namespace connection_index

//
// ConnectionIndex
//

template <>
ConnectionIndex<connection_index::ContentType::LogicItem,
                connection_index::DirectionType::Input>::ConnectionIndex(
    const Layout& layout) {
    generate_logicitem_messages(*this, layout);
}

template <>
ConnectionIndex<connection_index::ContentType::LogicItem,
                connection_index::DirectionType::Output>::ConnectionIndex(
    const Layout& layout) {
    generate_logicitem_messages(*this, layout);
}

template <>
ConnectionIndex<connection_index::ContentType::Wire,
                connection_index::DirectionType::Input>::ConnectionIndex(
    const Layout& layout) {
    generate_wire_messages(*this, layout);
}

template <>
ConnectionIndex<connection_index::ContentType::Wire,
                connection_index::DirectionType::Output>::ConnectionIndex(
    const Layout& layout) {
    generate_wire_messages(*this, layout);
}

//
// Format
//

template <>
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Input>::format() const
    -> std::string {
    return fmt::format("LogicItemInputIndex = {}", map_);
}

template <>
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Output>::format() const
    -> std::string {
    return fmt::format("LogicItemOutputIndex = {}", map_);
}

template <>
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Input>::format() const
    -> std::string {
    return fmt::format("WireInputIndex = {}", map_);
}

template <>
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Output>::format() const
    -> std::string {
    return fmt::format("WireOutputIndex = {}", map_);
}

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::allocated_size() const -> std::size_t {
    return get_allocated_size(map_);
}

namespace connection_index {

namespace {

auto get_and_verify_cache_entry(map_type<ContentType::LogicItem>& map, point_t position,
                                logicitem_connection_t value)
    -> map_type<ContentType::LogicItem>::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw std::runtime_error("unable to find cached data that should be present.");
    }
    return it;
}

auto get_and_verify_cache_entry(map_type<ContentType::Wire>& map, point_t position,
                                wire_value_t value)
    -> map_type<ContentType::Wire>::iterator {
    const auto it = map.find(position);
    if (it == map.end() || it->second != value) [[unlikely]] {
        throw std::runtime_error("unable to find cached data that should be present.");
    }
    return it;
}

}  // namespace

}  // namespace connection_index

//
// LogicItemInserted
//

template <>
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemInserted& message) -> void {
    using namespace connection_index;

    for (const auto info : input_locations_and_id(message.data)) {
        const auto [it, inserted] =
            map_.try_emplace(info.position, logicitem_connection_t {
                                                message.logicitem_id,
                                                info.input_id,
                                                info.orientation,
                                            });

        if (!inserted) [[unlikely]] {
            throw std::runtime_error("cache already has an entry at this position");
        }
    }
}

template <>
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemInserted& message) -> void {
    using namespace connection_index;

    for (auto info : output_locations_and_id(message.data)) {
        const auto [it, inserted] =
            map_.try_emplace(info.position, logicitem_connection_t {
                                                message.logicitem_id,
                                                info.output_id,
                                                info.orientation,
                                            });
        if (!inserted) [[unlikely]] {
            throw std::runtime_error("cache already has an entry at this position");
        }
    }
}

template <>
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemInserted&) -> void {}

template <>
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemInserted&) -> void {}

//
// InsertedLogicItemIdUpdated
//

template <>
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Input>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void {
    using namespace connection_index;

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
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Output>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated& message)
        -> void {
    using namespace connection_index;

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
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Input>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated&) -> void {}

template <>
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Output>::
    handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated&) -> void {}

//
// LogicItemUninserted
//

template <>
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    using namespace connection_index;

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
auto ConnectionIndex<connection_index::ContentType::LogicItem,
                     connection_index::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    using namespace connection_index;

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
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Input>::
    handle(const editable_circuit::info_message::LogicItemUninserted&) -> void {}

template <>
auto ConnectionIndex<connection_index::ContentType::Wire,
                     connection_index::DirectionType::Output>::
    handle(const editable_circuit::info_message::LogicItemUninserted&) -> void {}

//
// SegmentInserted
//

namespace connection_index {
namespace {

using namespace editable_circuit::info_message;

template <connection_index::DirectionType Direction>
consteval auto point_type() -> SegmentPointType {
    return Direction == DirectionType::Input ? SegmentPointType::input
                                             : SegmentPointType::output;
}

auto handle_wire(wire_map_t& map, SegmentPointType point_type,
                 const SegmentInserted& message) -> void {
    if (message.segment_info.p0_type == point_type) {
        const auto [it, inserted] = map.try_emplace(
            message.segment_info.line.p0,
            wire_value_t {
                .segment = message.segment,
                .orientation = to_orientation_p0(message.segment_info.line),
            });

        if (!inserted) [[unlikely]] {
            throw std::runtime_error("cache already has an entry at this position");
        }
    }

    if (message.segment_info.p1_type == point_type) {
        const auto [it, inserted] = map.try_emplace(
            message.segment_info.line.p1,
            wire_value_t {
                .segment = message.segment,
                .orientation = to_orientation_p1(message.segment_info.line),
            });

        if (!inserted) [[unlikely]] {
            throw std::runtime_error("cache already has an entry at this position");
        }
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
}  // namespace connection_index

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::handle(
    const editable_circuit::info_message::SegmentInserted& message) -> void {
    if constexpr (Content == ContentType::Wire) {
        using namespace connection_index;
        handle_wire(map_, point_type<Direction>(), message);
    }
}

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::handle(
    const editable_circuit::info_message::InsertedSegmentIdUpdated& message) -> void {
    if constexpr (Content == ContentType::Wire) {
        using namespace connection_index;
        handle_wire(map_, point_type<Direction>(), message);
    }
}

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::handle(
    const editable_circuit::info_message::InsertedEndPointsUpdated& message) -> void {
    using namespace editable_circuit::info_message;

    handle(SegmentUninserted {message.segment, message.old_segment_info});
    handle(SegmentInserted {message.segment, message.new_segment_info});
}

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::handle(
    const editable_circuit::info_message::SegmentUninserted& message) -> void {
    if constexpr (Content == ContentType::Wire) {
        using namespace connection_index;
        handle_wire(map_, point_type<Direction>(), message);
    }
}

//
// submit
//

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::submit(
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

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::find(point_t position) const
    -> std::optional<value_t> {
    if (const auto it = map_.find(position); it != map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::is_colliding(
    const layout_calculation_data_t& data) const -> bool
    requires(Content == ContentType::LogicItem)
{
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

template <connection_index::ContentType Content,
          connection_index::DirectionType Direction>
auto ConnectionIndex<Content, Direction>::validate(const Layout& layout) const -> void {
    auto cache = ConnectionIndex<Content, Direction> {layout};

    if (cache.map_ != this->map_) [[unlikely]] {
        throw std::runtime_error("current cache state doesn't match circuit");
    }
}

template class ConnectionIndex<connection_index::ContentType::LogicItem,
                               connection_index::DirectionType::Input>;
template class ConnectionIndex<connection_index::ContentType::LogicItem,
                               connection_index::DirectionType::Output>;

template class ConnectionIndex<connection_index::ContentType::Wire,
                               connection_index::DirectionType::Input>;
template class ConnectionIndex<connection_index::ContentType::Wire,
                               connection_index::DirectionType::Output>;

}  // namespace logicsim
