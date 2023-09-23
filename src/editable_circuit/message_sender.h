#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGE_SENDER_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGE_SENDER_H

#include "message_forward.h"

#include <functional>

namespace logicsim {

namespace editable_circuit {

constexpr static inline auto DEBUG_PRINT_HANDLER_INPUTS = false;
constexpr static inline auto DEBUG_PRINT_MESSAGES = false;

class MessageSender {
   public:
    [[nodiscard]] explicit MessageSender(
        std::function<void(const InfoMessage &)> callback);

    auto submit(const InfoMessage &message) -> void;

   private:
    std::function<void(const InfoMessage &)> callback_;
};

}  // namespace editable_circuit

}  // namespace logicsim

#endif
