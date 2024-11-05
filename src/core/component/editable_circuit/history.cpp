#include "core/component/editable_circuit/history.h"

#include "core/algorithm/fmt_join.h"
#include "core/algorithm/vector_operations.h"
#include "core/allocated_size/std_pair.h"
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
            return "decoration_create_temporary";
        case decoration_delete_temporary:
            return "decoration_delete_temporary";
        case decoration_move_temporary:
            return "decoration_move_temporary";
        case decoration_to_mode_temporary:
            return "decoration_to_mode_temporary";
        case decoration_to_mode_colliding:
            return "decoration_to_mode_colliding";
        case decoration_to_mode_insert:
            return "decoration_to_mode_insert";
        case decoration_change_attributes:
            return "decoration_change_attributes";

        case visible_selection_clear:
            return "visble_selection_clear";
        case visible_selection_set:
            return "visble_selection_set";
        case visible_selection_add:
            return "visible_selection_add";
        case visible_selection_update_last:
            return "visible_selection_update_last";
        case visible_selection_pop_last:
            return "visible_selection_pop_last";
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
        "    \n"
        "    decoration_keys = {},\n"
        "    placed_decorations = {},\n"
        "    move_delta_stack = {},\n"
        "    \n"
        "    visible_selections = {},\n"
        "    selection_rects = {},\n"
        "    selection_functions = {},\n"
        "  )",
        format_stack_vector(entries), decoration_keys,
        format_stack_vector(placed_decorations), move_deltas, selections.size(),
        selection_rects, selection_functions);
}

auto HistoryStack::allocated_size() const -> std::size_t {
    return get_allocated_size(entries) +             //
                                                     //
           get_allocated_size(decoration_keys) +     //
           get_allocated_size(placed_decorations) +  //
           get_allocated_size(move_deltas) +         //
                                                     //
           get_allocated_size(selections) +          //
           get_allocated_size(selection_rects) +     //
           get_allocated_size(selection_functions);
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

auto HistoryStack::push_decoration_create_temporary(
    decoration_key_t decoration_key, PlacedDecoration&& placed_decoration) -> void {
    // skip if it was just deleted
    if (get_back_vector(entries) == HistoryEntry::decoration_delete_temporary &&
        at_back_vector(decoration_keys) == decoration_key) {
        pop_back_vector(entries);
        pop_back_vector(decoration_keys);
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_create_temporary);
    decoration_keys.emplace_back(decoration_key);
    placed_decorations.emplace_back(std::move(placed_decoration));
}

auto HistoryStack::push_decoration_delete_temporary(decoration_key_t decoration_key)
    -> void {
    // skip if it was just created
    if (get_back_vector(entries) == HistoryEntry::decoration_create_temporary &&
        at_back_vector(decoration_keys) == decoration_key) {
        pop_back_vector(entries);
        pop_back_vector(decoration_keys);
        pop_back_vector(placed_decorations);
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_delete_temporary);
    decoration_keys.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_colliding_to_temporary(decoration_key_t decoration_key)
    -> void {
    // skip if it was just colliding
    if (get_back_vector(entries) == HistoryEntry::decoration_to_mode_colliding &&
        at_back_vector(decoration_keys) == decoration_key) {
        pop_back_vector(entries);
        pop_back_vector(decoration_keys);
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_to_mode_temporary);
    decoration_keys.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_temporary_to_colliding(decoration_key_t decoration_key)
    -> void {
    // skip if it was just temporary
    if (get_back_vector(entries) == HistoryEntry::decoration_to_mode_temporary &&
        at_back_vector(decoration_keys) == decoration_key) {
        pop_back_vector(entries);
        pop_back_vector(decoration_keys);
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_to_mode_colliding);
    decoration_keys.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_colliding_to_insert(decoration_key_t decoration_key)
    -> void {
    // skip if it was just colliding
    if (get_back_vector(entries) == HistoryEntry::decoration_to_mode_colliding &&
        at_back_vector(decoration_keys) == decoration_key) {
        pop_back_vector(entries);
        pop_back_vector(decoration_keys);
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_to_mode_insert);
    decoration_keys.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_insert_to_colliding(decoration_key_t decoration_key)
    -> void {
    // skip if it was just inserted
    if (get_back_vector(entries) == HistoryEntry::decoration_to_mode_insert &&
        at_back_vector(decoration_keys) == decoration_key) {
        pop_back_vector(entries);
        pop_back_vector(decoration_keys);
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_to_mode_colliding);
    decoration_keys.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_move_temporary(decoration_key_t decoration_key, int dx,
                                                  int dy) -> void {
    entries.emplace_back(HistoryEntry::decoration_move_temporary);
    decoration_keys.emplace_back(decoration_key);
    move_deltas.emplace_back(dx, dy);
}

auto HistoryStack::push_decoration_change_attributes(
    decoration_key_t decoration_key, PlacedDecoration&& placed_decoration) -> void {
    // skip similar changes
    if (last_non_group_entry(entries) == HistoryEntry::decoration_change_attributes &&
        at_back_vector(decoration_keys) == decoration_key) {
        return;
    }

    entries.emplace_back(HistoryEntry::decoration_change_attributes);
    decoration_keys.emplace_back(decoration_key);
    placed_decorations.emplace_back(std::move(placed_decoration));
}

//
// History
//

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

//
// Free Functions
//

auto get_entry_before_skip(const std::vector<HistoryEntry>& entries,
                           HistoryEntry skip_type) -> std::optional<HistoryEntry> {
    const auto view = entries | std::ranges::views::reverse;
    const auto it = std::ranges::find_if(
        view, [skip_type](const HistoryEntry& entry) { return entry != skip_type; });

    return it == view.end() ? std::nullopt : std::make_optional(*it);
}

auto last_non_group_entry(const std::vector<HistoryEntry>& entries)
    -> std::optional<HistoryEntry> {
    return get_entry_before_skip(entries, HistoryEntry::new_group);
}

}  // namespace editable_circuit

}  // namespace logicsim
