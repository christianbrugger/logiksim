#include "core/component/editable_circuit/history.h"

#include "core/allocated_size/trait.h"

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

namespace editable_circuit {

//
// History
//

auto History::format() const -> std::string {
    return fmt::format(
        "UndoHistory(\n"
        "  state = {}\n"
        "  undo_stack = {}\n"
        "  redo_stack = {}\n"
        ")",
        state, undo_stack, redo_stack, redo_stack);
}

auto History::allocated_size() const -> std::size_t {
    return get_allocated_size(undo_stack) +  //
           get_allocated_size(redo_stack);
}

auto History::get_stack() -> HistoryStack* {
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
