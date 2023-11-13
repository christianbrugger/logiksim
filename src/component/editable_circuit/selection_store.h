#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_STORE_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_SELECTION_STORE_H

#include "editable_circuit/selection.h"
#include "format/struct.h"
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

    using value_type = Selection;
    using iterator = map_t::iterator;
    using const_iterator = map_t::const_iterator;

   private:
    [[nodiscard]] auto format() const -> std::string;
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

    [[nodiscard]] auto create() -> selection_id_t;
    auto remove(selection_id_t selection_id) -> void;

   private:
    map_t selections_ {};
    selection_id_t next_selection_key_ {0};
};

}  // namespace editable_circuit

}  // namespace logicsim

#endif
