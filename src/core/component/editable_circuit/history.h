#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_HISTORY_H

#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/vocabulary/decoration_key_t.h"
#include "core/vocabulary/placed_decoration.h"

#include <vector>

namespace logicsim {

namespace editable_circuit {

enum class UndoType : uint8_t {
    new_group,

    create_temporary_element,
    delete_temporary_element,

    move_temporary_element,

    to_insertion_temporary,
    to_insertion_colliding,
    to_insertion_insert,

    change_attributes,
};

}

template <>
[[nodiscard]] auto format(editable_circuit::UndoType type) -> std::string;

namespace editable_circuit {

struct DecorationUndoEntry {
    decoration_key_t key {null_decoration_key};
    point_t position {};
    UndoType type {UndoType::create_temporary_element};

    [[nodiscard]] auto operator==(const DecorationUndoEntry&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<DecorationUndoEntry>);

struct CircuitHistory {
    [[nodiscard]] auto operator==(const CircuitHistory&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    std::vector<DecorationUndoEntry> undo_stack {};
    std::vector<PlacedDecoration> decoration_graveyard {};
};

static_assert(std::regular<CircuitHistory>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
