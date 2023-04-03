#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H

#include "format.h"
#include "layout_calculation_type.h"
#include "segment_tree.h"
#include "vocabulary.h"

#include <gsl/gsl>

#include <string>
#include <variant>

namespace logicsim {

class EditableCircuit;

namespace editable_circuit {

//
// Info Messages
//

namespace info_message {

// Any type of circuit elements

struct ElementCreated {
    element_id_t element_id;

    [[nodiscard]] auto format() const -> std::string;
};

struct ElementDeleted {
    element_id_t element_id;

    [[nodiscard]] auto format() const -> std::string;
};

struct ElementUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;

    [[nodiscard]] auto format() const -> std::string;
};

// Logic Items

struct LogicItemInserted {
    element_id_t element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemUninserted {
    element_id_t element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedLogicItemUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto format() const -> std::string;
};

// Wire Segments

struct SegmentCreated {
    segment_t segment;

    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentDeleted {
    segment_t segment;

    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentUpdated {
    segment_t new_segment;
    segment_t old_segment;

    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentInserted {
    segment_t segment;
    segment_info_t segment_info;

    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentUninserted {
    segment_t segment;
    segment_info_t segment_info;

    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedSegmentUpdated {
    segment_t new_segment;
    segment_t old_segment;
    segment_info_t segment_info;

    [[nodiscard]] auto format() const -> std::string;
};

// only inserted?
struct SegmentMerged {
    segment_t segment_from;
    segment_t segment_to;

    [[nodiscard]] auto format() const -> std::string;
};

// only inserted?
struct SegmentSplit {
    segment_t segment_from;
    segment_part_t part_from;
    segment_t segment_to;

    [[nodiscard]] auto format() const -> std::string;
};

using Message = std::variant<                                          //
    ElementCreated, ElementDeleted, ElementUpdated,                    //
    LogicItemInserted, LogicItemUninserted, InsertedLogicItemUpdated,  //
    SegmentCreated, SegmentDeleted, SegmentUpdated,                    //
    SegmentInserted, SegmentUninserted, InsertedSegmentUpdated,        //
    SegmentMerged, SegmentSplit>;

}  // namespace info_message

using InfoMessage = info_message::Message;
static_assert(sizeof(InfoMessage) == 56);

//
// MessageSender
//

class MessageSender {
   public:
    [[nodiscard]] explicit MessageSender(EditableCircuit &) noexcept;

    auto submit(InfoMessage message) -> void;

   private:
    gsl::not_null<EditableCircuit *> editable_circuit_;
};

static_assert(sizeof(MessageSender) == 8);

}  // namespace editable_circuit

}  // namespace logicsim

#endif