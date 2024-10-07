#include "layout_message.h"

#include <fmt/core.h>

namespace logicsim {

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

// Decoration

auto DecorationCreated::format() const -> std::string {
    return fmt::format("DecorationCreated(decoration_id = {})", decoration_id);
}

auto DecorationIdUpdated::format() const -> std::string {
    return fmt::format(
        "DecorationIdUpdated(new_decoration_id = {}, old_decoration_id = {})",
        new_decoration_id, old_decoration_id);
}

auto DecorationDeleted::format() const -> std::string {
    return fmt::format("DecorationDeleted(decoration_id = {})", decoration_id);
}

// Inserted Decoration

auto DecorationInserted::format() const -> std::string {
    return fmt::format("DecorationInserted(decoration_id = {})", decoration_id);
}

auto InsertedDecorationIdUpdated::format() const -> std::string {
    return fmt::format(
        "InsertedDecorationIdUpdated(new_decoration_id = {}, old_decoration_id = {})",
        new_decoration_id, old_decoration_id);
}

auto DecorationUninserted::format() const -> std::string {
    return fmt::format("DecorationUninserted(decoration_id = {})", decoration_id);
}

// Segments

auto SegmentCreated::format() const -> std::string {
    return fmt::format("SegmentCreated(segment = {}, size = {})", segment, size);
}

auto SegmentIdUpdated::format() const -> std::string {
    return fmt::format("SegmentIdUpdated(new_segment = {}, old_segment = {})",
                       new_segment, old_segment);
}

auto SegmentPartMoved::format() const -> std::string {
    return fmt::format("SegmentPartMoved(destination = {}, source = {})", destination,
                       source);
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

}  // namespace logicsim
