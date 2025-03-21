#ifndef LOGIKSIM_LAYOUT_MESSAGE_H
#define LOGIKSIM_LAYOUT_MESSAGE_H

#include "core/algorithm/fmt_join.h"
#include "core/format/container.h"
#include "core/format/struct.h"
#include "core/layout_message_forward.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/decoration_layout_data.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/segment.h"
#include "core/vocabulary/segment_info.h"
#include "core/vocabulary/segment_part.h"

#include <fmt/core.h>

#include <string>
#include <variant>

namespace logicsim {

//
// Info Messages
//

namespace info_message {

// Logic Items

struct LogicItemCreated {
    logicitem_id_t logicitem_id;

    [[nodiscard]] auto operator==(const LogicItemCreated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemIdUpdated {
    logicitem_id_t new_logicitem_id;
    logicitem_id_t old_logicitem_id;

    [[nodiscard]] auto operator==(const LogicItemIdUpdated &other) const -> bool =
                                                                                default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemDeleted {
    logicitem_id_t logicitem_id;

    [[nodiscard]] auto operator==(const LogicItemDeleted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Inserted Logic Items

struct LogicItemInserted {
    logicitem_id_t logicitem_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const LogicItemInserted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedLogicItemIdUpdated {
    logicitem_id_t new_logicitem_id;
    logicitem_id_t old_logicitem_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const InsertedLogicItemIdUpdated &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct LogicItemUninserted {
    logicitem_id_t logicitem_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto operator==(const LogicItemUninserted &other) const -> bool =
                                                                                 default;
    [[nodiscard]] auto format() const -> std::string;
};

// Decorations

struct DecorationCreated {
    decoration_id_t decoration_id;

    [[nodiscard]] auto operator==(const DecorationCreated &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct DecorationIdUpdated {
    decoration_id_t new_decoration_id;
    decoration_id_t old_decoration_id;

    [[nodiscard]] auto operator==(const DecorationIdUpdated &other) const -> bool =
                                                                                 default;
    [[nodiscard]] auto format() const -> std::string;
};

struct DecorationDeleted {
    decoration_id_t decoration_id;

    [[nodiscard]] auto operator==(const DecorationDeleted &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

// Inserted Decorations

struct DecorationInserted {
    decoration_id_t decoration_id;
    decoration_layout_data_t data;

    [[nodiscard]] auto operator==(const DecorationInserted &other) const -> bool =
                                                                                default;
    [[nodiscard]] auto format() const -> std::string;
};

struct InsertedDecorationIdUpdated {
    decoration_id_t new_decoration_id;
    decoration_id_t old_decoration_id;
    decoration_layout_data_t data;

    [[nodiscard]] auto operator==(const InsertedDecorationIdUpdated &other) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct DecorationUninserted {
    decoration_id_t decoration_id;
    decoration_layout_data_t data;

    [[nodiscard]] auto operator==(const DecorationUninserted &other) const -> bool =
                                                                                  default;
    [[nodiscard]] auto format() const -> std::string;
};

// Segments

struct SegmentCreated {
    segment_t segment;
    offset_t size;

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
    segment_part_t destination;
    segment_part_t source;
    bool create_destination;
    bool delete_source;

    [[nodiscard]] auto operator==(const SegmentPartMoved &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct SegmentPartDeleted {
    segment_part_t segment_part;
    bool delete_segment;

    [[nodiscard]] auto operator==(const SegmentPartDeleted &other) const -> bool =
                                                                                default;
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
    DecorationCreated, DecorationDeleted, DecorationIdUpdated,               //
    DecorationInserted, DecorationUninserted, InsertedDecorationIdUpdated,   //
    SegmentCreated, SegmentIdUpdated, SegmentPartMoved, SegmentPartDeleted,  //
    SegmentInserted, InsertedSegmentIdUpdated, InsertedEndPointsUpdated,
    SegmentUninserted>;

}  // namespace info_message

using InfoMessage = info_message::Message;

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::InfoMessage> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::InfoMessage &obj, fmt::format_context &ctx) const {
        const auto str = std::visit([](auto &&v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

template <>
struct fmt::formatter<logicsim::message_vector_t> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::message_vector_t &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", logicsim::fmt_join("\n", obj));
    }
};

#endif
