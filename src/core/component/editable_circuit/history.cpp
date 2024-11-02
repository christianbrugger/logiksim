#include "core/component/editable_circuit/history.h"

#include "core/algorithm/fmt_join.h"
#include "core/allocated_size/std_vector.h"
#include "core/format/container.h"
#include "core/format/std_type.h"

namespace logicsim {

template <>
auto format(editable_circuit::HistoryState state) -> std::string {
    using namespace editable_circuit;

    switch (state) {
        using enum HistoryState;

        case disabled:
            return "disabled";

        case track_undo:
            return "track_undo";
        case track_redo:
            return "track_redo";
    };
    std::terminate();
}

template <>
auto format(editable_circuit::UndoType type) -> std::string {
    using namespace editable_circuit;

    switch (type) {
        using enum UndoType;

        case new_group:
            return "new_group";

        case create_temporary_element:
            return "create_temporary_element";
        case delete_temporary_element:
            return "delete_temporary_element";

        case move_temporary_element:
            return "move_temporary_element";

        case to_insertion_temporary:
            return "to_insertion_temporary";
        case to_insertion_colliding:
            return "to_insertion_colliding";
        case to_insertion_insert:
            return "to_insertion_insert";

        case change_attributes:
            return "change_attributes";
    };
    std::terminate();
}

namespace editable_circuit {

auto DecorationUndoEntry::format() const -> std::string {
    return fmt::format("DecorationUndoEntry(type = {}, key = {}, position = {})", type,
                       key, position);
}

auto CircuitHistory::format() const -> std::string {
    const auto entry_str = fmt_join(",\n    ", undo_stack);
    const auto graveyard_str = fmt_join(",\n    ", decoration_graveyard);

    return fmt::format(
        "UndoHistory(\n"
        "  state = {}\n"
        "  decoration_undo_entries = [\n"
        "    {}\n"
        "  ],\n"
        "  decoration_graveyard = [\n"
        "    {}\n"
        "  ],\n"
        ")",
        state, entry_str, graveyard_str);
}

auto CircuitHistory::allocated_size() const -> std::size_t {
    return get_allocated_size(undo_stack) +  //
           get_allocated_size(decoration_graveyard);
}

}  // namespace editable_circuit

}  // namespace logicsim
