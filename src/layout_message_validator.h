#ifndef LOGICSIM_LAYOUT_MESSAGE_VALIDATOR_H
#define LOGICSIM_LAYOUT_MESSAGE_VALIDATOR_H

#include "format/container.h"
#include "format/std_type.h"
#include "format/struct.h"
#include "layout_message_forward.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/part.h"
#include "vocabulary/segment.h"
#include "vocabulary/segment_info.h"

#include <ankerl/unordered_dense.h>

#include <optional>

namespace logicsim {

class Layout;

namespace message_validator {

struct all_logicitem_value_t {
    uint64_t unique_id;

    [[nodiscard]] auto format() const -> std::string;
};

struct inserted_logicitem_value_t {
    uint64_t unique_id;
    layout_calculation_data_t data;

    [[nodiscard]] auto format() const -> std::string;
};

struct all_segment_value_t {
    uint64_t unique_id;
    part_t part;

    [[nodiscard]] auto format() const -> std::string;
};

struct inserted_segment_value_t {
    uint64_t unique_id;
    segment_info_t segment_info;

    [[nodiscard]] auto format() const -> std::string;
};

using all_logicitem_map_t =
    ankerl::unordered_dense::map<logicitem_id_t, all_logicitem_value_t>;
using inserted_logicitem_map_t =
    ankerl::unordered_dense::map<logicitem_id_t, inserted_logicitem_value_t>;

using all_segment_map_t = ankerl::unordered_dense::map<segment_t, all_segment_value_t>;
using inserted_segment_map_t =
    ankerl::unordered_dense::map<segment_t, inserted_segment_value_t>;

}  // namespace message_validator

/**
 * @brief: Validates that a stream messages are consistent.
 *
 * Note this is a pre-condition for components receiving
 * layout message updates. Sending messages to this class ensures this.
 */
class MessageValidator {
   public:
    using all_logicitem_value_t = message_validator::all_logicitem_value_t;
    using all_logicitem_map_t = message_validator::all_logicitem_map_t;

    using inserted_logicitem_value_t = message_validator::inserted_logicitem_value_t;
    using inserted_logicitem_map_t = message_validator::inserted_logicitem_map_t;

    using all_segment_value_t = message_validator::all_segment_value_t;
    using all_segment_map_t = message_validator::all_segment_map_t;

    using inserted_segment_value_t = message_validator::inserted_segment_value_t;
    using inserted_segment_map_t = message_validator::inserted_segment_map_t;

   public:
    [[nodiscard]] explicit MessageValidator() = default;
    [[nodiscard]] explicit MessageValidator(const Layout &layout);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto layout_matches_state(const Layout &layout) const -> bool;

    auto submit(const InfoMessage &message) -> void;

   private:
    [[nodiscard]] auto get_next_unique_id() -> uint64_t;

   private:
    auto handle(const info_message::LogicItemCreated &message) -> void;
    auto handle(const info_message::LogicItemIdUpdated &message) -> void;
    auto handle(const info_message::LogicItemDeleted &message) -> void;

    auto handle(const info_message::LogicItemInserted &message) -> void;
    auto handle(const info_message::InsertedLogicItemIdUpdated &message) -> void;
    auto handle(const info_message::LogicItemUninserted &message) -> void;

    auto handle(const info_message::SegmentCreated &message) -> void;
    auto handle(const info_message::SegmentIdUpdated &message) -> void;
    auto handle(const info_message::SegmentPartMoved &message) -> void;
    auto handle(const info_message::SegmentPartDeleted &message) -> void;

    auto handle(const info_message::SegmentInserted &message) -> void;
    auto handle(const info_message::InsertedSegmentIdUpdated &message) -> void;
    auto handle(const info_message::InsertedEndPointsUpdated &message) -> void;
    auto handle(const info_message::SegmentUninserted &message) -> void;

   private:
    uint64_t next_unique_id_ {};

    all_logicitem_map_t all_logicitems_ {};
    inserted_logicitem_map_t inserted_logicitems_ {};

    all_segment_map_t all_segments_ {};
    inserted_segment_map_t inserted_segments_ {};
};

}  // namespace logicsim

#endif
