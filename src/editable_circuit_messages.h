#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H

#include "vocabulary.h"

#include <variant>

namespace logicsim {

class EditableCircuit;

namespace editable_circuit {

//
// Info Messages
//

namespace info_message {

struct ElementCreated {
    element_id_t element_id;
};

struct ElementDeleted {
    element_id_t element_id;
};

struct ElementUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;
};

struct ElementInserted {
    element_id_t element_id;
};

struct ElementUninserted {
    element_id_t element_id;
};

struct SegmentInserted {
    segment_t segment;
};

struct SegmentUninserted {
    segment_t segment;
};

struct SegmentMerged {
    segment_t segment_from;
    segment_t segment_to;
};

struct SegmentSplit {
    segment_t segment_from;
    segment_part_t part_from;
    segment_t segment_to;
};

using Message = std::variant<ElementCreated, ElementDeleted, ElementUpdated,
                             ElementInserted, ElementUninserted, SegmentInserted,
                             SegmentUninserted, SegmentMerged, SegmentSplit>;

}  // namespace info_message

using InfoMessage = info_message::Message;
static_assert(sizeof(InfoMessage) == 24);

//
// MessageSender
//

class MessageSender {
   public:
    [[nodiscard]] explicit MessageSender(EditableCircuit &) noexcept;

    auto submit(InfoMessage &&m) -> void;

   private:
    gsl::not_null<EditableCircuit *> editable_circuit_;
};

static_assert(sizeof(MessageSender) == 8);

}  // namespace editable_circuit

}  // namespace logicsim

#endif