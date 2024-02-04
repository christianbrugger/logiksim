#ifndef LOGICSIM_LAYOUT_MESSAGE_VALIDATOR_H
#define LOGICSIM_LAYOUT_MESSAGE_VALIDATOR_H

#include "layout_message_forward.h"

namespace logicsim {

class Layout;

/**
 * @brief: Validates that a stream messages are consistent.
 *
 * Note this is a pre-condition for components receiving
 * layout message updates. Sending messages to this class ensures this.
 */
class MessageValidator {
   public:
    [[nodiscard]] explicit MessageValidator() = default;
    [[nodiscard]] explicit MessageValidator(const Layout &layout);

    auto submit(const InfoMessage &message) -> void;

   private:
    auto handle(const info_message::LogicItemInserted &message) -> void;
    auto handle(const info_message::LogicItemUninserted &message) -> void;
    auto handle(const info_message::InsertedLogicItemIdUpdated &message) -> void;

    auto handle(const info_message::SegmentInserted &message) -> void;
    auto handle(const info_message::SegmentUninserted &message) -> void;
    auto handle(const info_message::InsertedSegmentIdUpdated &message) -> void;

   private:
};

}  // namespace logicsim

#endif
