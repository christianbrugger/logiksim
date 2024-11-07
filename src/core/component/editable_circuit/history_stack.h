#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_STACK_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_STACK_H

#include "core/component/editable_circuit/visible_selection.h"
#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/stable_selection.h"
#include "core/vocabulary/decoration_key_t.h"
#include "core/vocabulary/move_delta.h"
#include "core/vocabulary/placed_decoration.h"

#include <utility>
#include <vector>

namespace logicsim {

namespace editable_circuit {

enum class HistoryEntry : uint8_t {
    new_group,

    // decoration
    decoration_create_temporary,
    decoration_delete_temporary,
    decoration_move_temporary,
    decoration_to_mode_temporary,
    decoration_to_mode_colliding,
    decoration_to_mode_insert,
    decoration_change_attributes,
    decoration_add_visible_selection,
    decoration_remove_visible_selection,

    // visible selection
    visible_selection_clear,
    visible_selection_set,
    visible_selection_add_operation,
    visible_selection_update_last,
    visible_selection_pop_last,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::HistoryEntry type) -> std::string;

namespace editable_circuit {

/**
 * @brief: Store history actions of the editable circuit.
 *
 * Class-invariants:
 *  + All substacks have correct size to fit the entries in the main stack.
 */
class HistoryStack {
   public:
    [[nodiscard]] auto operator==(const HistoryStack&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    auto clear() -> void;

    [[nodiscard]] auto top_entry() const -> std::optional<HistoryEntry>;

    //
    // Groups
    //

    auto push_new_group() -> bool;
    auto pop_new_group() -> void;

    //
    // Decoration
    //

    auto push_decoration_create_temporary(decoration_key_t decoration_key,
                                          PlacedDecoration&& placed_decoration) -> void;
    auto push_decoration_delete_temporary(decoration_key_t decoration_key) -> void;
    auto push_decoration_colliding_to_temporary(decoration_key_t decoration_key) -> void;
    auto push_decoration_temporary_to_colliding(decoration_key_t decoration_key) -> void;
    auto push_decoration_colliding_to_insert(decoration_key_t decoration_key) -> void;
    auto push_decoration_insert_to_colliding(decoration_key_t decoration_key) -> void;
    auto push_decoration_move_temporary(decoration_key_t decoration_key,
                                        move_delta_t delta) -> void;
    auto push_decoration_change_attributes(decoration_key_t decoration_key,
                                           attributes_text_element_t&& attrs) -> void;
    auto push_decoration_add_visible_selection(decoration_key_t decoration_key) -> void;
    auto push_decoration_remove_visible_selection(decoration_key_t decoration_key)
        -> void;

    auto pop_decoration_create_temporary()
        -> std::pair<decoration_key_t, PlacedDecoration>;
    auto pop_decoration_delete_temporary() -> decoration_key_t;
    auto pop_decoration_to_mode_temporary() -> decoration_key_t;
    auto pop_decoration_to_mode_colliding() -> decoration_key_t;
    auto pop_decoration_to_mode_insert() -> decoration_key_t;
    auto pop_decoration_move_temporary() -> std::pair<decoration_key_t, move_delta_t>;
    auto pop_decoration_change_attributes()
        -> std::pair<decoration_key_t, attributes_text_element_t>;
    auto pop_decoration_add_visible_selection() -> decoration_key_t;
    auto pop_decoration_remove_visible_selection() -> decoration_key_t;

    //
    // Visible Selection
    //

    auto push_visible_selection_clear() -> void;
    auto push_visible_selection_set(StableSelection&& stable_selection) -> void;
    auto push_visible_selection_add_operation(
        const VisibleSelection::operation_t& operation) -> void;
    auto push_visible_selection_update_last(const rect_fine_t& rect) -> void;
    auto push_visible_selection_pop_last() -> void;

    auto pop_visible_selection_clear() -> void;
    auto pop_visible_selection_set() -> StableSelection;
    auto pop_visible_selection_add_operation() -> visible_selection::operation_t;
    auto pop_visible_selection_update_last() -> rect_fine_t;
    auto pop_visible_selection_pop_last() -> void;

   private:
    std::vector<HistoryEntry> entries_ {};

    // decoration
    std::vector<decoration_key_t> decoration_keys_ {};
    std::vector<PlacedDecoration> placed_decorations_ {};
    std::vector<move_delta_t> move_deltas_ {};

    // visible selection
    std::vector<StableSelection> selections_ {};
    std::vector<rect_fine_t> selection_rects_ {};
    std::vector<SelectionFunction> selection_functions_ {};
};

static_assert(std::regular<HistoryStack>);

//
// Free Functions
//

[[nodiscard]] auto get_entry_before_skip(const std::vector<HistoryEntry>& entries,
                                         HistoryEntry skip_type)
    -> std::optional<HistoryEntry>;

[[nodiscard]] auto last_non_group_entry(const std::vector<HistoryEntry>& entries)
    -> std::optional<HistoryEntry>;

[[nodiscard]] auto has_ungrouped_entries(const HistoryStack& stack) -> bool;

auto reopen_group(HistoryStack& stack) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
