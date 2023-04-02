#include "editable_circuit_messages.h"

#include "exceptions.h"

namespace logicsim::editable_circuit {

//
// Info Messages
//

//
// Message Sender
//

MessageSender::MessageSender(EditableCircuit &editable_circuit) noexcept
    : editable_circuit_ {&editable_circuit} {}

auto MessageSender::submit(InfoMessage &&m) -> void {}

}  // namespace logicsim::editable_circuit
