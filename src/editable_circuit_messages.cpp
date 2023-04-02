#include "editable_circuit_messages.h"

#include "exceptions.h"

namespace logicsim::editable_circuit {

//
// Info Messages
//

namespace info_message {
/*
Abstract_ElementOrSegment::Abstract_ElementOrSegment(element_id_t element_id) noexcept
: element_id_ {element_id} {
if (!element_id) [[unlikely]] {
    throw_exception("invalid element_id");
}
}

Abstract_ElementOrSegment::Abstract_ElementOrSegment(segment_t segment) noexcept
: element_id_ {segment.element_id}, segment_index_ {segment.segment_index} {
if (!segment) [[unlikely]] {
    throw_exception("invalid segment");
}
}

auto Abstract_ElementOrSegment::is_element() const noexcept -> bool {
return !is_segment();
}

auto Abstract_ElementOrSegment::is_segment() const noexcept -> bool {
return bool {segment_index_};
}

auto Abstract_ElementOrSegment::element_id() const -> element_id_t {
if (!is_element()) [[unlikely]] {
    throw_exception("not an element");
}
return element_id_;
}

auto Abstract_ElementOrSegment::segment() const -> segment_t {
if (!is_segment()) [[unlikely]] {
    throw_exception("not an element");
}
return segment_t {element_id_, segment_index_};
}
*/

}  // namespace info_message

//
// Message Sender
//

MessageSender::MessageSender(EditableCircuit &editable_circuit) noexcept
    : editable_circuit_ {&editable_circuit} {}

auto MessageSender::submit(ActionMessage &&m) -> void {}

auto MessageSender::submit(InfoMessage &&m) -> void {}

}  // namespace logicsim::editable_circuit
