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

        case track_undo_new:
            return "track_undo_new";
        case track_undo_replay:
            return "track_undo_replay";
        case track_redo_replay:
            return "track_redo_replay";
    };
    std::terminate();
}

template <>
auto format(editable_circuit::HistoryEntry type) -> std::string {
    using namespace editable_circuit;

    switch (type) {
        using enum HistoryEntry;

        case new_group:
            return "new_group";

        case decoration_create_temporary:
            return "create_temporary_element";
        case decoration_delete_temporary:
            return "delete_temporary_element";

        case decoration_move_temporary:
            return "move_temporary_element";

        case decoration_to_insertion_temporary:
            return "to_insertion_temporary";
        case decoration_to_insertion_colliding:
            return "to_insertion_colliding";
        case decoration_to_insertion_insert:
            return "to_insertion_insert";

        case decoration_change_attributes:
            return "change_attributes";
    };
    std::terminate();
}

namespace editable_circuit {

namespace {

template <typename T>
auto format_stack_vector(const std::vector<T>& data) -> std::string {
    if (data.empty()) {
        return "[]";
    }
    return "[\n      " + fmt_join(",\n      ", data) + "\n    ]";
}

}  // namespace

auto HistoryStack::format() const -> std::string {
    return fmt::format(
        "Stack(\n"
        "    entries = {},\n"
        "    decoration_keys = {},\n"
        "    placed_decorations = {},\n"
        "    move_delta_stack = {},\n"
        "  )",
        format_stack_vector(entries), decoration_keys,
        format_stack_vector(placed_decorations), move_deltas);
}

auto HistoryStack::allocated_size() const -> std::size_t {
    return get_allocated_size(entries) +             //
           get_allocated_size(decoration_keys) +     //
           get_allocated_size(placed_decorations) +  //
           get_allocated_size(move_deltas);
}

auto HistoryStack::empty() const -> bool {
    return entries.empty();
}

auto HistoryStack::size() const -> std::size_t {
    return entries.size();
}

auto HistoryStack::clear() -> void {
    if (!empty()) {
        *this = HistoryStack {};
    }
}

auto CircuitHistory::format() const -> std::string {
    return fmt::format(
        "UndoHistory(\n"
        "  state = {}\n"
        "  undo_stack = {}\n"
        "  redo_stack = {}\n"
        ")",
        state, undo_stack, redo_stack, redo_stack);
}

auto CircuitHistory::allocated_size() const -> std::size_t {
    return get_allocated_size(undo_stack) +  //
           get_allocated_size(redo_stack);
}

auto CircuitHistory::get_stack() -> HistoryStack* {
    switch (state) {
        using enum HistoryState;
        case disabled: {
            return nullptr;
        }
        case track_undo_new: {
            redo_stack.clear();
            return &undo_stack;
        }
        case track_undo_replay: {
            return &undo_stack;
        }
        case track_redo_replay: {
            return &redo_stack;
        }
    };
    std::terminate();
}

}  // namespace editable_circuit

}  // namespace logicsim
