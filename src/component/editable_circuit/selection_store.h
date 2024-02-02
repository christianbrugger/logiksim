#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_STORE_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_STORE_H

#include "format/struct.h"
#include "layout_message_forward.h"
#include "selection.h"
#include "vocabulary/selection_id.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

namespace editable_circuit {

namespace selection_store {

using map_t = ankerl::unordered_dense::map<selection_id_t, Selection>;

}

/**
 * @brief: Key-Value Store for EditableCircuit Selections.
 */
class SelectionStore {
   public:
    using map_t = selection_store::map_t;

    using value_type = map_t::value_type;
    using iterator = map_t::iterator;
    using const_iterator = map_t::const_iterator;

   public:
    SelectionStore();

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto operator==(const SelectionStore& other) const -> bool = default;

    auto clear() -> void;
    auto empty() const -> bool;
    auto size() const -> std::size_t;

    [[nodiscard]] auto begin() -> iterator;
    [[nodiscard]] auto end() -> iterator;
    [[nodiscard]] auto begin() const -> const_iterator;
    [[nodiscard]] auto end() const -> const_iterator;

    [[nodiscard]] auto at(selection_id_t selection_id) -> Selection&;
    [[nodiscard]] auto at(selection_id_t selection_id) const -> const Selection&;

    [[nodiscard]] auto contains(selection_id_t selection_id) const -> bool;

    [[nodiscard]] auto create() -> selection_id_t;
    auto destroy(selection_id_t selection_id) -> void;

    auto submit(const editable_circuit::InfoMessage& message) -> void;

   private:
    selection_id_t next_selection_key_;

    map_t selections_ {};
};

static_assert(std::regular<SelectionStore>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
