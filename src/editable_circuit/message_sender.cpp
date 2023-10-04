#include "message_sender.h"

#include "editable_circuit/message.h"
#include "logging.h"

namespace logicsim {

namespace editable_circuit {

MessageSender::MessageSender(std::function<void(const InfoMessage &)> callback)
    : callback_ {std::move(callback)} {}

auto MessageSender::submit(const InfoMessage &message) -> void {
    if constexpr (DEBUG_PRINT_MESSAGES) {
        print(message);
    }
    callback_(message);
}

}  // namespace editable_circuit

}  // namespace logicsim