#include "editable_circuit/message.h"

#include "editable_circuit.h"
#include "exception.h"
#include "format.h"

namespace logicsim::editable_circuit {

//
// Info Messages
//

namespace info_message {

// Logic Item

auto LogicItemCreated::format() const -> std::string {
    return fmt::format("LogicItemCreated(element_id = {})", element_id);
}

auto LogicItemIdUpdated::format() const -> std::string {
    return fmt::format("LogicItemIdUpdated(new_element_id = {}, old_element_id = {})",
                       new_element_id, old_element_id);
}

auto LogicItemDeleted::format() const -> std::string {
    return fmt::format("LogicItemDeleted(element_id = {})", element_id);
}

// Inserted Logic Item

auto LogicItemInserted::format() const -> std::string {
    return fmt::format("LogicItemInserted(element_id = {})", element_id);
}

auto InsertedLogicItemIdUpdated::format() const -> std::string {
    return fmt::format(
        "InsertedLogicItemIdUpdated(new_element_id = {}, old_element_id = {})",
        new_element_id, old_element_id);
}

auto LogicItemUninserted::format() const -> std::string {
    return fmt::format("LogicItemUninserted(element_id = {})", element_id);
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

//
// Message Sender
//

// TransparentReceiver

TransparentReceiver::TransparentReceiver(EditableCircuit &editable_circuit)
    : editable_circuit_ {&editable_circuit} {}

auto TransparentReceiver::submit(InfoMessage message) -> void {
    editable_circuit_->_submit(message);
}

// Recording Reciever

AdaptableReceiver::AdaptableReceiver(VirtualReceiver &receiver) : receiver_ {&receiver} {}

AdaptableReceiver::AdaptableReceiver(EditableCircuit &editable_circuit)
    : editable_circuit_ {&editable_circuit} {}

auto AdaptableReceiver::submit(InfoMessage message) -> void {
#ifndef LOGIKSIM_TESTING_MESSEGE_SENDER
    throw_exception("using adaptable receiver in non-testing code");
#endif

    if (receiver_ != nullptr) {
        receiver_->submit(message);
    }
    if (editable_circuit_ != nullptr) {
        editable_circuit_->_submit(message);
    }
}

// Message Sender

MessageSender::MessageSender(MessageReceiver receiver) noexcept : receiver_ {receiver} {}

auto MessageSender::submit(InfoMessage message) -> void {
    if constexpr (DEBUG_PRINT_MESSAGES) {
        print(message);
    }
    receiver_.submit(message);
}

}  // namespace logicsim::editable_circuit
