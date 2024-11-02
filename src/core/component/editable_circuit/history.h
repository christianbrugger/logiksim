#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H

#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/vocabulary/decoration_key_t.h"
#include "core/vocabulary/placed_decoration.h"

#include <vector>

namespace logicsim {

namespace editable_circuit {

enum class HistoryState {
    disabled,

    track_undo,
    track_redo,
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
};

}

template <>
[[nodiscard]] auto format(editable_circuit::HistoryEntry type) -> std::string;

namespace editable_circuit {

struct HistoryStack {
    std::vector<HistoryEntry> entries {};
    std::vector<decoration_key_t> decoration_keys {};
    std::vector<PlacedDecoration> placed_decorations {};
    std::vector<std::pair<int, int>> move_deltas {};

    [[nodiscard]] auto operator==(const HistoryStack&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
};

static_assert(std::regular<HistoryStack>);

struct CircuitHistory {
    [[nodiscard]] auto operator==(const CircuitHistory&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    HistoryState state {HistoryState::disabled};

    HistoryStack undo_stack {};
    HistoryStack redo_stack {};

    [[nodiscard]] auto get_stack() const -> const HistoryStack*;
    [[nodiscard]] auto get_stack() -> HistoryStack*;
};

static_assert(std::regular<CircuitHistory>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
