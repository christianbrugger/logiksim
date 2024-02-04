#include "layout_message_validator.h"

#include "layout_message.h"
#include "layout_message_generation.h"

#include <fmt/core.h>
#include <gsl/gsl>

namespace logicsim {

namespace message_validator {

auto uninserted_logicitem_value_t::format() const -> std::string {
    return fmt::format("(id = {})", unique_id);
}

auto inserted_logicitem_value_t::format() const -> std::string {
    return fmt::format("(id = {}, data = {})", unique_id, data);
}

auto uninserted_segment_value_t::format() const -> std::string {
    return fmt::format("(id = {})", unique_id);
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
        "  uninserted_logicitems_ = {}\n"
        "  inserted_logicitems_ = {}\n"
        "  uninserted_segments_ = {}\n"
        "  inserted_segments_ = {}\n"
        "}}",
        uninserted_logicitems_, inserted_logicitems_, uninserted_segments_,
        inserted_segments_);
}

namespace message_validator {
namespace {

auto uninserted_logicitems_match(const uninserted_logicitem_map_t &map,
                                 const Layout &layout) -> bool {
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

}  // namespace

// namespace
}  // namespace message_validator

auto MessageValidator::layout_matches_state(const Layout &layout) const -> bool {
    using namespace message_validator;

    return uninserted_logicitems_match(uninserted_logicitems_, layout) &&
           inserted_logicitems_match(inserted_logicitems_, layout);
}

auto MessageValidator::submit(const InfoMessage &message) -> void {
    std::visit([this](const auto &message_) { this->handle(message_); }, message);
}

auto MessageValidator::get_next_unique_id() -> uint64_t {
    return next_unique_id_++;
}

//
// Logic Item
//

auto MessageValidator::handle(const info_message::LogicItemCreated &message) -> void {
    const auto value = uninserted_logicitem_value_t {
        .unique_id = get_next_unique_id(),
    };
    Expects(uninserted_logicitems_.emplace(message.logicitem_id, value).second);
}

auto MessageValidator::handle(const info_message::LogicItemIdUpdated &message) -> void {
    const auto value = uninserted_logicitems_.at(message.old_logicitem_id);

    Expects(uninserted_logicitems_.erase(message.old_logicitem_id) == 1);
    Expects(uninserted_logicitems_.emplace(message.new_logicitem_id, value).second);

    // check inserted unique_id
    if (const auto it = inserted_logicitems_.find(message.old_logicitem_id);
        it != inserted_logicitems_.end()) {
        Expects(it->second.unique_id == value.unique_id);
    }
}

auto MessageValidator::handle(const info_message::LogicItemDeleted &message) -> void {
    Expects(uninserted_logicitems_.erase(message.logicitem_id) == 1);
}

//
// Inserted Logic Item
//

auto MessageValidator::handle(const info_message::LogicItemInserted &message) -> void {
    const auto uninserted_unique_id =
        uninserted_logicitems_.at(message.logicitem_id).unique_id;

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
        uninserted_logicitems_.at(message.new_logicitem_id).unique_id;
    Expects(value.unique_id == uninserted_unique_id);
}

auto MessageValidator::handle(const info_message::LogicItemUninserted &message) -> void {
    const auto uninserted_unique_id =
        uninserted_logicitems_.at(message.logicitem_id).unique_id;

    const auto &value = inserted_logicitems_.at(message.logicitem_id);
    Expects(value.data == message.data);
    Expects(value.unique_id == uninserted_unique_id);

    Expects(inserted_logicitems_.erase(message.logicitem_id) == 1);
}

//
// Segment
//

auto MessageValidator::handle(const info_message::SegmentCreated &message) -> void {}

auto MessageValidator::handle(const info_message::SegmentIdUpdated &message) -> void {}

auto MessageValidator::handle(const info_message::SegmentPartMoved &message) -> void {}

auto MessageValidator::handle(const info_message::SegmentPartDeleted &message) -> void {}

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
