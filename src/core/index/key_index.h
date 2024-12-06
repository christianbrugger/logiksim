#ifndef LOGICSIM_CORE_INDEX_KEY_INDEX_H
#define LOGICSIM_CORE_INDEX_KEY_INDEX_H

#include "core/format/struct.h"
#include "core/layout_message_forward.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/decoration_key.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/logicitem_key.h"
#include "core/vocabulary/segment.h"
#include "core/vocabulary/segment_key.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

class Layout;

namespace key_index {

template <class Key, class Value>
using map_type = ankerl::unordered_dense::map<Key, Value>;

using map_logicitem_keys_t = map_type<logicitem_id_t, logicitem_key_t>;
using map_logicitem_ids_t = map_type<logicitem_key_t, logicitem_id_t>;

using map_decoration_keys_t = map_type<decoration_id_t, decoration_key_t>;
using map_decoration_ids_t = map_type<decoration_key_t, decoration_id_t>;

using map_segment_keys_t = map_type<segment_t, segment_key_t>;
using map_segment_ids_t = map_type<segment_key_t, segment_t>;

}  // namespace key_index

/**
 * @brief: Maintains unique keys for each circuit element and lookup.
 *
 * Pre-conditions:
 *   + requires a correct history of messages of element changes
 *
 * Class-invariants:
 *   + all used keys are smaller than next_key
 *   + key and id map are consistent with each other
 *   + key and id map only contain valid ids
 */
class KeyIndex {
   public:
    using map_logicitem_keys_t = key_index::map_logicitem_keys_t;
    using map_logicitem_ids_t = key_index::map_logicitem_ids_t;

    using map_decoration_keys_t = key_index::map_decoration_keys_t;
    using map_decoration_ids_t = key_index::map_decoration_ids_t;

    using map_segment_keys_t = key_index::map_segment_keys_t;
    using map_segment_ids_t = key_index::map_segment_ids_t;

   public:
    explicit KeyIndex() = default;
    explicit KeyIndex(const Layout& layout);

    [[nodiscard]] auto operator==(const KeyIndex&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    // logicitems
    [[nodiscard]] auto get(logicitem_id_t logicitem_id) const -> logicitem_key_t;
    [[nodiscard]] auto get(logicitem_key_t logicitem_key) const -> logicitem_id_t;
    auto set(logicitem_id_t logicitem_id, logicitem_key_t logicitem_key) -> void;
    // decorations
    [[nodiscard]] auto get(decoration_id_t decoration_id) const -> decoration_key_t;
    [[nodiscard]] auto get(decoration_key_t decoration_key) const -> decoration_id_t;
    auto set(decoration_id_t decoration_id, decoration_key_t decoration_key) -> void;
    // segments
    [[nodiscard]] auto get(segment_t segment) const -> segment_key_t;
    [[nodiscard]] auto get(segment_key_t segment_key) const -> segment_t;
    auto set(segment_t segment, segment_key_t segment_key) -> void;
    auto swap(segment_t segment_0, segment_t segment_1) -> void;

    [[nodiscard]] auto segments() const
        -> std::span<const std::pair<segment_t, segment_key_t>>;

    auto submit(const InfoMessage& message) -> void;

    [[nodiscard]] auto has_all_ids_inserted(const Layout& layout) const -> bool;

   private:
    auto handle(const info_message::LogicItemCreated& message) -> void;
    auto handle(const info_message::LogicItemIdUpdated& message) -> void;
    auto handle(const info_message::LogicItemDeleted& message) -> void;

    auto handle(const info_message::DecorationCreated& message) -> void;
    auto handle(const info_message::DecorationIdUpdated& message) -> void;
    auto handle(const info_message::DecorationDeleted& message) -> void;

    auto handle(const info_message::SegmentCreated& message) -> void;
    auto handle(const info_message::SegmentIdUpdated& message) -> void;
    auto handle(const info_message::SegmentPartMoved& message) -> void;
    auto handle(const info_message::SegmentPartDeleted& message) -> void;

    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    map_logicitem_keys_t logicitem_keys_ {};
    map_logicitem_ids_t logicitem_ids_ {};
    logicitem_key_t next_logicitem_key_ {0};

    map_decoration_keys_t decoration_keys_ {};
    map_decoration_ids_t decoration_ids_ {};
    decoration_key_t next_decoration_key_ {0};

    map_segment_keys_t segment_keys_ {};
    map_segment_ids_t segment_ids_ {};
    segment_key_t next_segment_key_ {0};
};

}  // namespace logicsim

#endif
