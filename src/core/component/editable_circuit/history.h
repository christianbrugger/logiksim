#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H

#include "core/component/editable_circuit/visible_selection.h"
#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/stable_selection.h"
#include "core/vocabulary/decoration_key_t.h"
#include "core/vocabulary/placed_decoration.h"

#include <optional>
#include <vector>

namespace logicsim {

namespace editable_circuit {

enum class HistoryState {
    disabled,

    track_undo_new,
    track_undo_replay,
    track_redo_replay,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::HistoryState state) -> std::string;

namespace editable_circuit {

enum class HistoryEntry : uint8_t {
    new_group,

    // decoration
    decoration_create_temporary,
    decoration_delete_temporary,
    decoration_move_temporary,
    decoration_to_insertion_temporary,
    decoration_to_insertion_colliding,
    decoration_to_insertion_insert,
    decoration_change_attributes,

    // visible selection
    visible_selection_clear,
    visible_selection_set,
    visible_selection_add,
    visible_selection_update_last,
    visible_selection_pop_last,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::HistoryEntry type) -> std::string;

namespace editable_circuit {

struct HistoryStack {
    std::vector<HistoryEntry> entries {};

    // decoration
    std::vector<decoration_key_t> decoration_keys {};
    std::vector<PlacedDecoration> placed_decorations {};
    std::vector<std::pair<int, int>> move_deltas {};

    // visible selection
    std::vector<StableSelection> selections {};
    std::vector<rect_fine_t> selection_rects {};
    std::vector<SelectionFunction> selection_functions {};

    [[nodiscard]] auto operator==(const HistoryStack&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    auto clear() -> void;
};

static_assert(std::regular<HistoryStack>);

struct CircuitHistory {
    HistoryState state {HistoryState::disabled};
    HistoryStack undo_stack {};
    HistoryStack redo_stack {};

    [[nodiscard]] auto operator==(const CircuitHistory&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto get_stack() -> HistoryStack*;
};

static_assert(std::regular<CircuitHistory>);

//
// Free Functions
//

[[nodiscard]] auto last_non_group_entry(const std::vector<HistoryEntry>& entries)
    -> std::optional<HistoryEntry>;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
