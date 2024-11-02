#ifndef LOGICSIM_CORE_INDEX_KEY_INDEX_H
#define LOGICSIM_CORE_INDEX_KEY_INDEX_H

#include "core/format/struct.h"
#include "core/layout_message_forward.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/decoration_key_t.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

class Layout;

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
    template <class Key, class Value>
    using map_type = ankerl::unordered_dense::map<Key, Value>;

   public:
    explicit KeyIndex() = default;
    explicit KeyIndex(const Layout& layout);

    [[nodiscard]] auto operator==(const KeyIndex&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    // return null if ids are not found
    [[nodiscard]] auto get(decoration_id_t decoration_id) const -> decoration_key_t;
    [[nodiscard]] auto get(decoration_key_t decoration_key) const -> decoration_id_t;
    auto set(decoration_id_t decoration_id, decoration_key_t decoration_key) -> void;

    auto submit(const InfoMessage& message) -> void;
    [[nodiscard]] auto has_all_ids_inserted(const Layout& layout) const -> bool;

   private:
    auto handle(const info_message::DecorationCreated& message) -> void;
    auto handle(const info_message::DecorationIdUpdated& message) -> void;
    auto handle(const info_message::DecorationDeleted& message) -> void;

    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    map_type<decoration_id_t, decoration_key_t> decoration_keys_ {};
    map_type<decoration_key_t, decoration_id_t> decoration_ids_ {};

    decoration_key_t next_decoration_key_ {0};
};

}  // namespace logicsim

#endif
