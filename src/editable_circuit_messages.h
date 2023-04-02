#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H

#include "vocabulary.h"

#include <variant>

namespace logicsim {

class EditableCircuit;

namespace editable_circuit {

//
// Action Messages
//

namespace action_message {

struct DeleteElement {
    element_id_t element_id;
};

using Message = std::variant<DeleteElement>;

}  // namespace action_message

using ActionMessage = action_message::Message;

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

/*
class Abstract_ElementOrSegment {
   public:
    explicit Abstract_ElementOrSegment(element_id_t element_id) noexcept;
    explicit Abstract_ElementOrSegment(segment_t segment) noexcept;

    [[nodiscard]] auto is_element() const noexcept -> bool;
    [[nodiscard]] auto is_segment() const noexcept -> bool;

    [[nodiscard]] auto element_id() const -> element_id_t;
    [[nodiscard]] auto segment() const -> segment_t;

   private:
    element_id_t element_id_ {null_element};
    segment_index_t segment_index_ {null_segment_index};
};
*/

class ElementInserted {
    element_id_t element_id;
};

class ElementUninserted {
    element_id_t element_id;
};

class SegmentInserted {
    segment_t element_id;
};

class SegmentUninserted {
    segment_t element_id;
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

//
// MessageSender
//

class MessageSender {
   public:
    [[nodiscard]] explicit MessageSender(EditableCircuit &) noexcept;

    auto submit(ActionMessage &&m) -> void;
    auto submit(InfoMessage &&m) -> void;

   private:
    gsl::not_null<EditableCircuit *> editable_circuit_;
};

}  // namespace editable_circuit

}  // namespace logicsim

#endif