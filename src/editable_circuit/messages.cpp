#include "editable_circuit/messages.h"

#include "editable_circuit.h"
#include "exceptions.h"
#include "format.h"

namespace logicsim::editable_circuit {

//
// Info Messages
//

namespace info_message {

auto ElementCreated::format() const -> std::string {
    return fmt::format("ElementCreated(element_id = {})", element_id);
}

auto ElementDeleted::format() const -> std::string {
    return fmt::format("ElementDeleted(element_id = {})", element_id);
}

auto ElementUpdated::format() const -> std::string {
    return fmt::format("ElementUpdated(new_element_id = {}, old_element_id = {})",
                       new_element_id, old_element_id);
}

// Logic Item

auto LogicItemInserted::format() const -> std::string {
    return fmt::format("LogicItemInserted(element_id = {})", element_id);
}

auto LogicItemUninserted::format() const -> std::string {
    return fmt::format("LogicItemUninserted(element_id = {})", element_id);
}

auto InsertedLogicItemUpdated::format() const -> std::string {
    return fmt::format(
        "InsertedLogicItemUpdated(new_element_id = {}, old_element_id = {})",
        new_element_id, old_element_id);
}

// Segments

auto SegmentCreated::format() const -> std::string {
    return fmt::format("SegmentCreated(segment = {})", segment);
}

auto SegmentDeleted::format() const -> std::string {
    return fmt::format("SegmentDeleted(segment = {})", segment);
}

auto SegmentUpdated::format() const -> std::string {
    return fmt::format("SegmentUpdated(new_segment = {}, old_segment = {})", new_segment,
                       old_segment);
}

auto SegmentInserted::format() const -> std::string {
    return fmt::format("SegmentInserted(segment = {})", segment);
}

auto SegmentUninserted::format() const -> std::string {
    return fmt::format("SegmentUninserted(segment = {})", segment);
}

auto InsertedSegmentUpdated::format() const -> std::string {
    return fmt::format("InsertedSegmentUpdated(new_segment = {}, old_segment = {})",
                       new_segment, old_segment);
}

auto SegmentMerged::format() const -> std::string {
    return fmt::format("SegmentMerged(segment_from = {}, segment_to = {})", segment_from,
                       segment_to);
}

auto SegmentSplit::format() const -> std::string {
    return fmt::format("SegmentSplit(segment_from = {}, part_from = {}, segment_to = {})",
                       segment_from, part_from, segment_to);
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
    receiver_.submit(message);
}

}  // namespace logicsim::editable_circuit
