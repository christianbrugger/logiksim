#include "editable_circuit/message.h"

#include "exception.h"

#include <fmt/core.h>

namespace logicsim::editable_circuit {

//
// Info Messages
//

namespace info_message {

// Logic Item

auto LogicItemCreated::format() const -> std::string {
    return fmt::format("LogicItemCreated(logicitem_id = {})", logicitem_id);
}

auto LogicItemIdUpdated::format() const -> std::string {
    return fmt::format("LogicItemIdUpdated(new_logicitem_id = {}, old_logicitem_id = {})",
                       new_logicitem_id, old_logicitem_id);
}

auto LogicItemDeleted::format() const -> std::string {
    return fmt::format("LogicItemDeleted(logicitem_id = {})", logicitem_id);
}

// Inserted Logic Item

auto LogicItemInserted::format() const -> std::string {
    return fmt::format("LogicItemInserted(logicitem_id = {})", logicitem_id);
}

auto InsertedLogicItemIdUpdated::format() const -> std::string {
    return fmt::format(
        "InsertedLogicItemIdUpdated(new_logicitem_id = {}, old_logicitem_id = {})",
        new_logicitem_id, old_logicitem_id);
}

auto LogicItemUninserted::format() const -> std::string {
    return fmt::format("LogicItemUninserted(logicitem_id = {})", logicitem_id);
}

// Segments

auto SegmentCreated::format() const -> std::string {
    return fmt::format("SegmentCreated(segment = {})", segment);
}

auto SegmentIdUpdated::format() const -> std::string {
    return fmt::format("SegmentIdUpdated(new_segment = {}, old_segment = {})",
                       new_segment, old_segment);
}

auto SegmentPartMoved::format() const -> std::string {
    return fmt::format(
        "SegmentPartMoved(segment_part_destination = {}, segment_part_source = {})",
        segment_part_destination, segment_part_source);
}

auto SegmentPartDeleted::format() const -> std::string {
    return fmt::format("SegmentPartDeleted(segment_part = {})", segment_part);
}

// Inserted Segments

auto SegmentInserted::format() const -> std::string {
    return fmt::format("SegmentInserted(segment = {}, segment_info = {})", segment,
                       segment_info);
}

auto InsertedSegmentIdUpdated::format() const -> std::string {
    return fmt::format(
        "InsertedSegmentIdUpdated(new_segment = {}, old_segment = {}, segment_info = {})",
        new_segment, old_segment, segment_info);
}

auto InsertedEndPointsUpdated::format() const -> std::string {
    return fmt::format(
        "InsertedEndPointsUpdated(segment = {}, new_segment_info = {}, "
        "old_segment_info = {})",
        segment, new_segment_info, old_segment_info);
}

auto SegmentUninserted::format() const -> std::string {
    return fmt::format("SegmentUninserted(segment = {}, segment_info = {})", segment,
                       segment_info);
}

}  // namespace info_message

}  // namespace logicsim::editable_circuit
