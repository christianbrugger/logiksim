#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGES_H

#include "editable_circuit/types.h"
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

// Logic Items

struct LogicItemCreated {
    element_id_t element_id;

    [[nodiscard]] auto operator==(const LogicItemCreated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemDeleted {
    element_id_t element_id;

    [[nodiscard]] auto operator==(const LogicItemDeleted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;

    [[nodiscard]] auto operator==(const LogicItemUpdated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Inserted Logic Items

struct LogicItemInserted {
    element_id_t element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const LogicItemInserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemUninserted {
    element_id_t element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const LogicItemUninserted &other) const -> bool
        = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedLogicItemUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const InsertedLogicItemUpdated &other) const -> bool
        = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Wire Segments

struct SegmentCreated {
    segment_t segment;

    [[nodiscard]] auto operator==(const SegmentCreated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentDeleted {
    segment_t segment;

    [[nodiscard]] auto operator==(const SegmentDeleted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentIdUpdated {
    segment_t new_segment;
    segment_t old_segment;

    [[nodiscard]] auto operator==(const SegmentIdUpdated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Inserted Wire Segments

struct SegmentInserted {
    segment_t segment;
    segment_info_t segment_info;

    [[nodiscard]] auto operator==(const SegmentInserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentUninserted {
    segment_t segment;
    segment_info_t segment_info;

    [[nodiscard]] auto operator==(const SegmentUninserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedSegmentUpdated {
    segment_t new_segment;
    segment_t old_segment;
    segment_info_t segment_info;

    [[nodiscard]] auto operator==(const InsertedSegmentUpdated &other) const -> bool
        = default;
    [[nodiscard]] auto format() const -> std::string;
};

// only inserted?
struct SegmentMerged {
    segment_t segment_from;
    segment_t segment_to;

    [[nodiscard]] auto operator==(const SegmentMerged &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// only inserted?
struct SegmentSplit {
    segment_t segment_from;
    part_t part_from;
    segment_t segment_to;

    [[nodiscard]] auto operator==(const SegmentSplit &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

using Message = std::variant<                                          //
    LogicItemCreated, LogicItemDeleted, LogicItemUpdated,              //
    LogicItemInserted, LogicItemUninserted, InsertedLogicItemUpdated,  //
    SegmentCreated, SegmentDeleted, SegmentIdUpdated,                  //
    SegmentInserted, SegmentUninserted, InsertedSegmentUpdated,        //
    SegmentMerged, SegmentSplit>;

}  // namespace info_message

using InfoMessage = info_message::Message;
static_assert(sizeof(InfoMessage) == 56);

//
// MessageSender
//

class TransparentReceiver {
   public:
    TransparentReceiver(EditableCircuit &editable_circuit);

    auto submit(InfoMessage message) -> void;

   private:
    gsl::not_null<EditableCircuit *> editable_circuit_;
};

// used for Testing
class VirtualReceiver {
   public:
    virtual ~VirtualReceiver() = default;
    virtual auto submit(InfoMessage message) -> void = 0;
};

// used for Testing
class AdaptableReceiver {
   public:
    AdaptableReceiver(VirtualReceiver &receiver);
    AdaptableReceiver(EditableCircuit &editable_circuit);

    auto submit(InfoMessage message) -> void;

   private:
    VirtualReceiver *receiver_ {nullptr};
    EditableCircuit *editable_circuit_ {nullptr};
};

#ifndef LOGIKSIM_TESTING_MESSEGE_SENDER
using MessageReceiver = TransparentReceiver;
#else
using MessageReceiver = AdaptableReceiver;
#endif

class MessageSender {
   public:
    [[nodiscard]] explicit MessageSender(MessageReceiver) noexcept;

    auto submit(InfoMessage message) -> void;

   private:
    MessageReceiver receiver_;
};

#ifndef LOGIKSIM_TESTING_MESSEGE_SENDER
static_assert(sizeof(MessageSender) == 8);
#endif

}  // namespace editable_circuit

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::editable_circuit::InfoMessage> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::editable_circuit::InfoMessage &obj,
                fmt::format_context &ctx) const {
        const auto str = std::visit([](auto &&v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

#endif
