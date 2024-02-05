#include "layout_message_validator.h"

#include "layout_message.h"
#include "layout_message_generation.h"
#include "logging.h"

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

auto all_segment_value_t::format() const -> std::string {
    return fmt::format("(id = {}, part = {})", unique_id, part);
}

auto inserted_segment_value_t::format() const -> std::string {
    return fmt::format("(id = {}, TODO )", unique_id);
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

namespace message_validator {
namespace {

auto all_logicitems_match(const all_logicitem_map_t &map, const Layout &layout) -> bool {
    const auto entry_matches = [&](const auto &logicitem_id) {
        return map.contains(logicitem_id);
    };

    return map.size() == layout.logic_items().size() &&
           std::ranges::all_of(logicitem_ids(layout), entry_matches);
}

auto inserted_logicitems_match(const inserted_logicitem_map_t &map, const Layout &layout)
    -> bool {
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

}  // namespace

// namespace
}  // namespace message_validator

auto MessageValidator::layout_matches_state(const Layout &layout) const -> bool {
    using namespace message_validator;

    return all_logicitems_match(all_logicitems_, layout) &&
           inserted_logicitems_match(inserted_logicitems_, layout) &&
           logicitem_unique_ids_match(all_logicitems_, inserted_logicitems_) &&
           // segments
           all_segments_match(all_segments_, layout);
}

auto MessageValidator::get_next_unique_id() -> uint64_t {
    return next_unique_id_++;
}

auto MessageValidator::submit(const InfoMessage &message) -> void {
    // print("::", message);
    // print(*this);

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
}

//
// Inserted Logic Item
//

auto MessageValidator::handle(const info_message::LogicItemInserted &message) -> void {
    const auto uninserted_unique_id = all_logicitems_.at(message.logicitem_id).unique_id;

    const auto value = inserted_logicitem_value_t {
        .unique_id = uninserted_unique_id,
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
    const auto uninserted_unique_id =
        all_logicitems_.at(message.new_logicitem_id).unique_id;
    Expects(value.unique_id == uninserted_unique_id);
}

auto MessageValidator::handle(const info_message::LogicItemUninserted &message) -> void {
    const auto uninserted_unique_id = all_logicitems_.at(message.logicitem_id).unique_id;

    const auto &value = inserted_logicitems_.at(message.logicitem_id);
    Expects(value.data == message.data);
    Expects(value.unique_id == uninserted_unique_id);

    Expects(inserted_logicitems_.erase(message.logicitem_id) == 1);
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
}

auto MessageValidator::handle(const info_message::SegmentIdUpdated &message) -> void {
    const auto value = all_segments_.at(message.old_segment);

    Expects(all_segments_.erase(message.old_segment) == 1);
    Expects(all_segments_.emplace(message.new_segment, value).second);

    //// check inserted unique_id
    // if (const auto it = inserted_logicitems_.find(message.old_logicitem_id);
    //     it != inserted_logicitems_.end()) {
    //     Expects(it->second.unique_id == value.unique_id);
    // }
}

auto MessageValidator::handle(const info_message::SegmentPartMoved &message) -> void {
    // adapt source
    {
        auto &source = all_segments_.at(message.source.segment);

        if (message.source.part.begin == source.part.begin &&
            message.source.part.end == source.part.end) {
            // source completely deleted
            Expects(all_segments_.erase(message.source.segment) == 1);
        } else if (message.source.part.begin == source.part.begin) {
            // shrinking front
            source.part = part_t {message.source.part.end, source.part.end};
        } else if (message.source.part.end == source.part.end) {
            // shrinking back
            source.part = part_t {source.part.begin, message.source.part.begin};
        } else {
            std::terminate();
        }
    }

    // adapt destination
    const auto it = all_segments_.find(message.destination.segment);

    if (it == all_segments_.end()) {
        // new destination
        const auto value = all_segment_value_t {
            .unique_id = get_next_unique_id(),
            .part = message.destination.part,
        };
        Expects(all_segments_.emplace(message.destination.segment, value).second);
    } else if (it->second.part.begin == message.destination.part.end) {
        // expanding front
        it->second.part = part_t {message.destination.part.begin, it->second.part.end};
    } else if (it->second.part.end == message.destination.part.begin) {
        // expanding back
        it->second.part = part_t {it->second.part.begin, message.destination.part.end};
    } else {
        std::terminate();
    }
}

auto MessageValidator::handle(const info_message::SegmentPartDeleted &message) -> void {
    auto &value = all_segments_.at(message.segment_part.segment);

    if (message.segment_part.part.begin == message.segment_part.part.begin &&
        message.segment_part.part.end == message.segment_part.part.end) {
        // delete complete segment
        Expects(all_segments_.erase(message.segment_part.segment) == 1);
    } else if (message.segment_part.part.begin == message.segment_part.part.begin) {
        // shrink front
        value.part = part_t {message.segment_part.part.end, value.part.end};
    } else if (message.segment_part.part.end == message.segment_part.part.end) {
        // shrink back
        value.part = part_t {value.part.begin, message.segment_part.part.begin};
    } else {
        std::terminate();
    }
}

//
// Inserted Segment
//

auto MessageValidator::handle(const info_message::SegmentInserted &message) -> void {}

auto MessageValidator::handle(const info_message::InsertedSegmentIdUpdated &message)
    -> void {}

auto MessageValidator::handle(const info_message::InsertedEndPointsUpdated &message)
    -> void {}

auto MessageValidator::handle(const info_message::SegmentUninserted &message) -> void {}

}  // namespace logicsim
