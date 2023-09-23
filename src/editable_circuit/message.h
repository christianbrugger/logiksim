#ifndef LOGIKSIM_EDITABLE_CIRCUIT_MESSAGE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_MESSAGE_H

#include "format.h"
#include "layout_calculation_type.h"
#include "segment_tree_type.h"
#include "vocabulary.h"

#include <gsl/gsl>

#include <functional>
#include <string>
#include <variant>

namespace logicsim {

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

struct LogicItemIdUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;

    [[nodiscard]] auto operator==(const LogicItemIdUpdated &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemDeleted {
    element_id_t element_id;

    [[nodiscard]] auto operator==(const LogicItemDeleted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Inserted Logic Items

struct LogicItemInserted {
    element_id_t element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const LogicItemInserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedLogicItemIdUpdated {
    element_id_t new_element_id;
    element_id_t old_element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const InsertedLogicItemIdUpdated &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemUninserted {
    element_id_t element_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const LogicItemUninserted &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Segments

struct SegmentCreated {
    segment_t segment;

    [[nodiscard]] auto operator==(const SegmentCreated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentIdUpdated {
    segment_t new_segment;
    segment_t old_segment;

    [[nodiscard]] auto operator==(const SegmentIdUpdated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentPartMoved {
    segment_part_t segment_part_destination;
    segment_part_t segment_part_source;

    [[nodiscard]] auto operator==(const SegmentPartMoved &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentPartDeleted {
    segment_part_t segment_part;

    [[nodiscard]] auto operator==(const SegmentPartDeleted &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Inserted Segments

struct SegmentInserted {
    segment_t segment;
    segment_info_t segment_info;

    [[nodiscard]] auto operator==(const SegmentInserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedSegmentIdUpdated {
    segment_t new_segment;
    segment_t old_segment;
    segment_info_t segment_info;

    [[nodiscard]] auto operator==(const InsertedSegmentIdUpdated &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// updates in meta data of endpoints, not positions
struct InsertedEndPointsUpdated {
    segment_t segment;
    segment_info_t new_segment_info;
    segment_info_t old_segment_info;

    [[nodiscard]] auto operator==(const InsertedEndPointsUpdated &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentUninserted {
    segment_t segment;
    segment_info_t segment_info;

    [[nodiscard]] auto operator==(const SegmentUninserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

using Message = std::variant<                                                //
    LogicItemCreated, LogicItemDeleted, LogicItemIdUpdated,                  //
    LogicItemInserted, LogicItemUninserted, InsertedLogicItemIdUpdated,      //
    SegmentCreated, SegmentIdUpdated, SegmentPartMoved, SegmentPartDeleted,  //
    SegmentInserted, InsertedSegmentIdUpdated, InsertedEndPointsUpdated,
    SegmentUninserted>;

}  // namespace info_message

using InfoMessage = info_message::Message;
static_assert(sizeof(InfoMessage) == 48);

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
