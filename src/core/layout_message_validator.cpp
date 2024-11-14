#include "core/layout_message_validator.h"

#include "core/allocated_size/ankerl_unordered_dense.h"
#include "core/layout_message.h"
#include "core/layout_message_generation.h"
#include "core/logging.h"

#include <fmt/core.h>
#include <gsl/gsl>

namespace logicsim {

namespace message_validator {

auto all_logicitem_value_t::format() const -> std::string {
    return fmt::format("(id = {})", unique_id);
}

auto inserted_logicitem_value_t::format() const -> std::string {
    return fmt::format("(id = {}, data = {})", unique_id, data);
}

auto all_decoration_value_t::format() const -> std::string {
    return fmt::format("(id = {})", unique_id);
}

auto inserted_decoration_value_t::format() const -> std::string {
    return fmt::format("(id = {}, data = {})", unique_id, data);
}

auto all_segment_value_t::format() const -> std::string {
    return fmt::format("(id = {}, part = {})", unique_id, part);
}

auto inserted_segment_value_t::format() const -> std::string {
    return fmt::format("(id = {}, segment_info = {} )", unique_id, segment_info);
}

}  // namespace message_validator

//
// Message Validator
//

MessageValidator::MessageValidator(const Layout &layout) {
    generate_all_layout_messages(*this, layout);
}

auto MessageValidator::format() const -> std::string {
    return fmt::format(
        "MessageValidator{{\n"
        "  all_logicitems_ = {}\n"
        "  inserted_logicitems_ = {}\n"
        "  all_segments_ = {}\n"
        "  inserted_segments_ = {}\n"
        "}}",
        all_logicitems_, inserted_logicitems_, all_segments_, inserted_segments_);
}

auto MessageValidator::allocated_size() const -> std::size_t {
    return get_allocated_size(all_logicitems_) +       //
           get_allocated_size(inserted_logicitems_) +  //
           get_allocated_size(all_segments_) +         //
           get_allocated_size(inserted_segments_);
}

namespace message_validator {
namespace {

auto all_logicitems_match(const all_logicitem_map_t &map, const Layout &layout) -> bool {
    const auto entry_matches = [&](const auto &logicitem_id) {
        return map.contains(logicitem_id);
    };

    return map.size() == layout.logicitems().size() &&
           std::ranges::all_of(logicitem_ids(layout), entry_matches);
}

auto inserted_logicitems_match(const inserted_logicitem_map_t &map,
                               const Layout &layout) -> bool {
    const auto data_matches = [&](const auto &logicitem_id) {
        return map.at(logicitem_id).data ==
               to_layout_calculation_data(layout, logicitem_id);
    };

    const auto entry_matches = [&](const auto &logicitem_id) {
        return !is_inserted(layout, logicitem_id) || data_matches(logicitem_id);
    };

    return map.size() == get_inserted_logicitem_count(layout) &&
           std::ranges::all_of(logicitem_ids(layout), entry_matches);
}

auto logicitem_unique_ids_match(const all_logicitem_map_t &all_items,
                                const inserted_logicitem_map_t &inserted) -> bool {
    return std::ranges::all_of(
        inserted, [&](const inserted_logicitem_map_t::value_type &item) {
            return all_items.at(item.first).unique_id == item.second.unique_id;
        });
}

auto all_segments_match(const all_segment_map_t &map, const Layout &layout) -> bool {
    const auto segment_matches = [&](const segment_t &segment) {
        return map.at(segment).part ==
               layout.wires().segment_tree(segment.wire_id).part(segment.segment_index);
    };

    const auto wire_matches = [&](const wire_id_t &wire_id) {
        return std::ranges::all_of(layout.wires().segment_tree(wire_id).indices(wire_id),
                                   segment_matches);
    };

    return std::ranges::all_of(wire_ids(layout), wire_matches);
}

auto inserted_segments_match(const inserted_segment_map_t &map,
                             const Layout &layout) -> bool {
    const auto segment_matches = [&](const segment_t &segment) {
        return map.at(segment).segment_info == get_segment_info(layout, segment);
    };

    const auto wire_matches = [&](const wire_id_t &wire_id) {
        return std::ranges::all_of(layout.wires().segment_tree(wire_id).indices(wire_id),
                                   segment_matches);
    };

    return std::ranges::all_of(inserted_wire_ids(layout), wire_matches);
}

auto segment_data_matches(const all_segment_map_t &all_segments,
                          const inserted_segment_map_t &inserted) -> bool {
    const auto data_matches = [&](const inserted_segment_map_t::value_type &item) {
        const auto &uninserted = all_segments.at(item.first);

        return uninserted.unique_id == item.second.unique_id &&
               uninserted.part == to_part(item.second.segment_info.line);
    };

    return std::ranges::all_of(inserted, data_matches);
}

}  // namespace

// namespace
}  // namespace message_validator

auto MessageValidator::layout_matches_state(const Layout &layout) const -> bool {
    using namespace message_validator;

    return all_logicitems_match(all_logicitems_, layout) &&
           inserted_logicitems_match(inserted_logicitems_, layout) &&
           logicitem_unique_ids_match(all_logicitems_, inserted_logicitems_) &&
           // segments
           all_segments_match(all_segments_, layout) &&
           inserted_segments_match(inserted_segments_, layout) &&
           segment_data_matches(all_segments_, inserted_segments_);
}

auto MessageValidator::get_next_unique_id() -> uint64_t {
    return next_unique_id_++;
}

auto MessageValidator::submit(const InfoMessage &message) -> void {
    std::visit([this](const auto &message_) { this->handle(message_); }, message);
}

//
// Logic Item
//

auto MessageValidator::handle(const info_message::LogicItemCreated &message) -> void {
    const auto value = all_logicitem_value_t {
        .unique_id = get_next_unique_id(),
    };
    Expects(all_logicitems_.emplace(message.logicitem_id, value).second);

    // not inserted yet
    Expects(!inserted_logicitems_.contains(message.logicitem_id));
}

auto MessageValidator::handle(const info_message::LogicItemIdUpdated &message) -> void {
    const auto value = all_logicitems_.at(message.old_logicitem_id);

    Expects(all_logicitems_.erase(message.old_logicitem_id) == 1);
    Expects(all_logicitems_.emplace(message.new_logicitem_id, value).second);

    // check inserted unique_id
    if (const auto it = inserted_logicitems_.find(message.old_logicitem_id);
        it != inserted_logicitems_.end()) {
        Expects(it->second.unique_id == value.unique_id);
    }
}

auto MessageValidator::handle(const info_message::LogicItemDeleted &message) -> void {
    Expects(all_logicitems_.erase(message.logicitem_id) == 1);

    // not inserted anymore
    Expects(!inserted_logicitems_.contains(message.logicitem_id));
}

//
// Inserted Logic Item
//

auto MessageValidator::handle(const info_message::LogicItemInserted &message) -> void {
    const auto &uninserted = all_logicitems_.at(message.logicitem_id);

    const auto value = inserted_logicitem_value_t {
        .unique_id = uninserted.unique_id,
        .data = message.data,
    };
    Expects(inserted_logicitems_.emplace(message.logicitem_id, value).second);
}

auto MessageValidator::handle(const info_message::InsertedLogicItemIdUpdated &message)
    -> void {
    const auto value = inserted_logicitems_.at(message.old_logicitem_id);
    Expects(value.data == message.data);

    Expects(inserted_logicitems_.erase(message.old_logicitem_id) == 1);
    Expects(inserted_logicitems_.emplace(message.new_logicitem_id, value).second);

    // check uninserted unique id
    const auto uninserted = all_logicitems_.at(message.new_logicitem_id);
    Expects(value.unique_id == uninserted.unique_id);
}

auto MessageValidator::handle(const info_message::LogicItemUninserted &message) -> void {
    const auto &value = inserted_logicitems_.at(message.logicitem_id);
    Expects(value.data == message.data);

    // check uninserted unique id
    const auto uninserted = all_logicitems_.at(message.logicitem_id);
    Expects(value.unique_id == uninserted.unique_id);

    Expects(inserted_logicitems_.erase(message.logicitem_id) == 1);
}

//
// Decoration
//

auto MessageValidator::handle(const info_message::DecorationCreated &message) -> void {
    const auto value = all_decoration_value_t {
        .unique_id = get_next_unique_id(),
    };
    Expects(all_decorations_.emplace(message.decoration_id, value).second);

    // not inserted yet
    Expects(!inserted_decorations_.contains(message.decoration_id));
}

auto MessageValidator::handle(const info_message::DecorationIdUpdated &message) -> void {
    const auto value = all_decorations_.at(message.old_decoration_id);

    Expects(all_decorations_.erase(message.old_decoration_id) == 1);
    Expects(all_decorations_.emplace(message.new_decoration_id, value).second);

    // check inserted unique_id
    if (const auto it = inserted_decorations_.find(message.old_decoration_id);
        it != inserted_decorations_.end()) {
        Expects(it->second.unique_id == value.unique_id);
    }
}

auto MessageValidator::handle(const info_message::DecorationDeleted &message) -> void {
    Expects(all_decorations_.erase(message.decoration_id) == 1);

    // not inserted anymore
    Expects(!inserted_decorations_.contains(message.decoration_id));
}

//
// Inserted Decoration
//

auto MessageValidator::handle(const info_message::DecorationInserted &message) -> void {
    const auto &uninserted = all_decorations_.at(message.decoration_id);

    const auto value = inserted_decoration_value_t {
        .unique_id = uninserted.unique_id,
        .data = message.data,
    };
    Expects(inserted_decorations_.emplace(message.decoration_id, value).second);
}

auto MessageValidator::handle(const info_message::InsertedDecorationIdUpdated &message)
    -> void {
    const auto value = inserted_decorations_.at(message.old_decoration_id);
    Expects(value.data == message.data);

    Expects(inserted_decorations_.erase(message.old_decoration_id) == 1);
    Expects(inserted_decorations_.emplace(message.new_decoration_id, value).second);

    // check uninserted unique id
    const auto uninserted = all_decorations_.at(message.new_decoration_id);
    Expects(value.unique_id == uninserted.unique_id);
}

auto MessageValidator::handle(const info_message::DecorationUninserted &message) -> void {
    const auto &value = inserted_decorations_.at(message.decoration_id);
    Expects(value.data == message.data);

    // check uninserted unique id
    const auto uninserted = all_decorations_.at(message.decoration_id);
    Expects(value.unique_id == uninserted.unique_id);

    Expects(inserted_decorations_.erase(message.decoration_id) == 1);
}

//
// Segment
//

auto MessageValidator::handle(const info_message::SegmentCreated &message) -> void {
    Expects(message.size > offset_t {0});

    const auto value = all_segment_value_t {
        .unique_id = get_next_unique_id(),
        .part = part_t {offset_t {0}, message.size},
    };
    Expects(all_segments_.emplace(message.segment, value).second);

    // not inserted yet
    Expects(!inserted_segments_.contains(message.segment));
}

auto MessageValidator::handle(const info_message::SegmentIdUpdated &message) -> void {
    const auto value = all_segments_.at(message.old_segment);

    Expects(all_segments_.erase(message.old_segment) == 1);
    Expects(all_segments_.emplace(message.new_segment, value).second);

    // check inserted data
    if (const auto it = inserted_segments_.find(message.old_segment);
        it != inserted_segments_.end()) {
        Expects(value.unique_id == it->second.unique_id);
        Expects(value.part == to_part(it->second.segment_info.line));
    }
}

auto MessageValidator::handle(const info_message::SegmentPartMoved &message) -> void {
    Expects(distance(message.destination.part) == distance(message.source.part));

    // adapt source
    {
        auto &source = all_segments_.at(message.source.segment);

        if (message.source.segment == message.destination.segment) {
            // source moved
            Expects(!message.delete_source);
            Expects(message.source.part == source.part);
            source.part = message.destination.part;
        } else if (message.source.part == source.part) {
            // source completely deleted
            Expects(message.delete_source);
            Expects(all_segments_.erase(message.source.segment) == 1);
        } else if (message.source.part.begin == source.part.begin) {
            // shrinking front
            Expects(!message.delete_source);
            source.part = part_t {message.source.part.end, source.part.end};
        } else if (message.source.part.end == source.part.end) {
            // shrinking back
            Expects(!message.delete_source);
            source.part = part_t {source.part.begin, message.source.part.begin};
        } else {
            std::terminate();
        }
    }

    // adapt destination
    if (message.source.segment != message.destination.segment) {
        const auto it = all_segments_.find(message.destination.segment);

        if (it == all_segments_.end()) {
            // new destination
            Expects(message.create_destination);
            const auto value = all_segment_value_t {
                .unique_id = get_next_unique_id(),
                .part = message.destination.part,
            };
            Expects(all_segments_.emplace(message.destination.segment, value).second);
        } else if (it->second.part.begin == message.destination.part.end) {
            // expanding front
            Expects(!message.create_destination);
            it->second.part =
                part_t {message.destination.part.begin, it->second.part.end};
        } else if (it->second.part.end == message.destination.part.begin) {
            // expanding back
            Expects(!message.create_destination);
            it->second.part =
                part_t {it->second.part.begin, message.destination.part.end};
        } else {
            std::terminate();
        }
    } else {
        Expects(!message.create_destination);
    }

    // segments are not inserted during move
    Expects(!inserted_segments_.contains(message.source.segment));
    Expects(!inserted_segments_.contains(message.destination.segment));
}

auto MessageValidator::handle(const info_message::SegmentPartDeleted &message) -> void {
    auto &value = all_segments_.at(message.segment_part.segment);

    if (message.segment_part.part.begin == message.segment_part.part.begin &&
        message.segment_part.part.end == message.segment_part.part.end) {
        // delete complete segment
        Expects(message.delete_segment);
        Expects(all_segments_.erase(message.segment_part.segment) == 1);
    } else if (message.segment_part.part.begin == message.segment_part.part.begin) {
        // shrink front
        Expects(!message.delete_segment);
        value.part = part_t {message.segment_part.part.end, value.part.end};
    } else if (message.segment_part.part.end == message.segment_part.part.end) {
        // shrink back
        Expects(!message.delete_segment);
        value.part = part_t {value.part.begin, message.segment_part.part.begin};
    } else {
        std::terminate();
    }

    // segment is not inserted during deletion
    Expects(!inserted_segments_.contains(message.segment_part.segment));
}

//
// Inserted Segment
//

auto MessageValidator::handle(const info_message::SegmentInserted &message) -> void {
    // check uninserted data
    const auto &uninserted = all_segments_.at(message.segment);
    Expects(uninserted.part == to_part(message.segment_info.line));

    const auto value = inserted_segment_value_t {
        .unique_id = uninserted.unique_id,
        .segment_info = message.segment_info,
    };
    Expects(inserted_segments_.emplace(message.segment, value).second);
}

auto MessageValidator::handle(const info_message::InsertedSegmentIdUpdated &message)
    -> void {
    const auto value = inserted_segments_.at(message.old_segment);
    Expects(value.segment_info == message.segment_info);

    Expects(inserted_segments_.erase(message.old_segment) == 1);
    Expects(inserted_segments_.emplace(message.new_segment, value).second);

    // check uninserted data
    const auto &uninserted = all_segments_.at(message.new_segment);
    Expects(uninserted.unique_id == value.unique_id);
    Expects(uninserted.part == to_part(value.segment_info.line));
}

auto MessageValidator::handle(const info_message::InsertedEndPointsUpdated &message)
    -> void {
    auto &value = inserted_segments_.at(message.segment);
    Expects(value.segment_info == message.old_segment_info);
    // can only change endpoints
    Expects(value.segment_info.line == message.new_segment_info.line);

    // update endpoints
    value.segment_info = segment_info_t {
        .line = value.segment_info.line,
        .p0_type = message.new_segment_info.p0_type,
        .p1_type = message.new_segment_info.p1_type,
    };
    Expects(value.segment_info == message.new_segment_info);

    // check uninserted data
    const auto &uninserted = all_segments_.at(message.segment);
    Expects(uninserted.unique_id == value.unique_id);
    Expects(uninserted.part == to_part(value.segment_info.line));
}

auto MessageValidator::handle(const info_message::SegmentUninserted &message) -> void {
    const auto &value = inserted_segments_.at(message.segment);
    Expects(value.segment_info == message.segment_info);

    // check uninserted data
    const auto &uninserted = all_segments_.at(message.segment);
    Expects(uninserted.unique_id == value.unique_id);
    Expects(uninserted.part == to_part(value.segment_info.line));

    Expects(inserted_segments_.erase(message.segment) == 1);
}

}  // namespace logicsim
