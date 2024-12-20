#include "core/component/editable_circuit/history_stack.h"

#include "core/algorithm/fmt_join.h"
#include "core/algorithm/numeric.h"
#include "core/algorithm/vector_operations.h"
#include "core/allocated_size/std_vector.h"
#include "core/format/container.h"
#include "core/geometry/segment_info.h"

namespace logicsim {

template <>
auto format(editable_circuit::HistoryEntry type) -> std::string {
    using namespace editable_circuit;

    switch (type) {
        using enum HistoryEntry;

        case new_group:
            return "new_group";

        case logicitem_create_temporary:
            return "logicitem_create_temporary";
        case logicitem_delete_temporary:
            return "logicitem_delete_temporary";
        case logicitem_move_temporary:
            return "logicitem_move_temporary";
        case logicitem_to_mode_temporary:
            return "logicitem_to_mode_temporary";
        case logicitem_to_mode_colliding_expect_valid:
            return "logicitem_to_mode_colliding_expect_valid";
        case logicitem_to_mode_colliding_assume_colliding:
            return "logicitem_to_mode_colliding_assume_colliding";
        case logicitem_to_mode_insert:
            return "logicitem_to_mode_insert";
        case logicitem_change_attributes:
            return "logicitem_change_attributes";
        case logicitem_add_visible_selection:
            return "logicitem_add_visible_selection";
        case logicitem_remove_visible_selection:
            return "logicitem_remove_visible_selection";

        case decoration_create_temporary:
            return "decoration_create_temporary";
        case decoration_delete_temporary:
            return "decoration_delete_temporary";
        case decoration_move_temporary:
            return "decoration_move_temporary";
        case decoration_to_mode_temporary:
            return "decoration_to_mode_temporary";
        case decoration_to_mode_colliding_expect_valid:
            return "decoration_to_mode_colliding_expect_valid";
        case decoration_to_mode_colliding_assume_colliding:
            return "decoration_to_mode_colliding_assume_colliding";
        case decoration_to_mode_insert:
            return "decoration_to_mode_insert";
        case decoration_change_attributes:
            return "decoration_change_attributes";
        case decoration_add_visible_selection:
            return "decoration_add_visible_selection";
        case decoration_remove_visible_selection:
            return "decoration_remove_visible_selection";

        case segment_create_temporary:
            return "segment_create_temporary";
        case segment_delete_temporary:
            return "segment_delete_temporary";
        case segment_move_temporary:
            return "segment_move_temporary";
        case segment_to_mode_temporary:
            return "segment_to_mode_temporary";
        case segment_to_mode_colliding_expect_valid:
            return "segment_to_mode_colliding_expect_valid";
        case segment_to_mode_colliding_assume_colliding:
            return "segment_to_mode_colliding_assume_colliding";
        case segment_to_mode_insert:
            return "segment_to_mode_insert";
        case segment_set_endpoints:
            return "segment_set_endpoints";
        case segment_merge:
            return "segment_merge";
        case segment_split:
            return "segment_split";
        case segment_add_visible_selection:
            return "segment_add_visible_selection";

        case visible_selection_clear:
            return "visble_selection_clear";
        case visible_selection_set:
            return "visble_selection_set";
        case visible_selection_add_operation:
            return "visible_selection_add_operation";
        case visible_selection_update_last:
            return "visible_selection_update_last";
        case visible_selection_pop_last:
            return "visible_selection_pop_last";
        case visible_selection_select_all:
            return "visible_selection_select_all";
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

auto split_segment_key_t::format() const -> std::string {
    return fmt::format(
        "split_segment_key_t{{source = {}, new_key = {}, split_offset = {}}}", source,
        new_key, split_offset);
}

auto HistoryStack::format() const -> std::string {
    return fmt::format(
        "Stack(\n"
        "    group_count = {},\n"
        "    entries = {},\n"
        "    move_delta_stack = {},\n"
        "    \n"
        "    logicitem_keys = {},\n"
        "    placed_logicitems = {},\n"
        "    \n"
        "    decoration_keys = {},\n"
        "    placed_decorations = {},\n"
        "    \n"
        "    segment_keys = {},\n"
        "    lines = {},\n"
        "    endpoints = {},\n"
        "    parts = {},\n"
        "    offsets = {},\n"
        "    \n"
        "    visible_selections = {},\n"
        "    selection_rects = {},\n"
        "    selection_functions = {},\n"
        "  )",
        group_count_, format_stack_vector(entries_), move_deltas_,   //
        logicitem_keys_, format_stack_vector(placed_logicitems_),    //
        decoration_keys_, format_stack_vector(placed_decorations_),  //
        segment_keys_, lines_, endpoints_, parts_, offsets_,         //
        format_stack_vector(selections_), selection_rects_, selection_functions_);
}

auto HistoryStack::allocated_size() const -> std::size_t {
    return get_allocated_size(entries_) +             //
           get_allocated_size(move_deltas_) +         //
                                                      //
           get_allocated_size(logicitem_keys_) +      //
           get_allocated_size(placed_logicitems_) +   //
                                                      //
           get_allocated_size(decoration_keys_) +     //
           get_allocated_size(placed_decorations_) +  //
                                                      //
           get_allocated_size(segment_keys_) +        //
           get_allocated_size(lines_) +               //
           get_allocated_size(endpoints_) +           //
           get_allocated_size(parts_) +               //
           get_allocated_size(offsets_) +             //
                                                      //
           get_allocated_size(selections_) +          //
           get_allocated_size(selection_rects_) +     //
           get_allocated_size(selection_functions_);
}

auto HistoryStack::empty() const -> bool {
    return entries_.empty();
}

auto HistoryStack::size() const -> std::size_t {
    return entries_.size();
}

auto HistoryStack::clear() -> void {
    if (!empty()) {
        *this = HistoryStack {};
    }
}

auto HistoryStack::top_entry() const -> std::optional<HistoryEntry> {
    return get_back_vector(entries_);
}

//
// Groups
//

auto HistoryStack::push_new_group() -> bool {
    if (!has_ungrouped_entries(*this)) {
        return false;
    }

    entries_.emplace_back(HistoryEntry::new_group);
    group_count_ = checked_add(group_count_, std::size_t {1});
    return true;
}

auto HistoryStack::pop_new_group() -> void {
    Expects(pop_back_vector(entries_) == HistoryEntry::new_group);
    group_count_ = checked_sub(group_count_, std::size_t {1});
}

auto HistoryStack::group_count() const -> std::size_t {
    return group_count_;
}

//
// Logic Items
//

auto HistoryStack::push_logicitem_create_temporary(
    logicitem_key_t logicitem_key, PlacedLogicItem&& placed_logicitem) -> void {
    // optimize so mouse insertion does not produce endless entries
    if (get_back_vector(entries_) == HistoryEntry::logicitem_delete_temporary &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        pop_logicitem_delete_temporary();
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_create_temporary);
    logicitem_keys_.emplace_back(logicitem_key);
    placed_logicitems_.emplace_back(std::move(placed_logicitem));
}

auto HistoryStack::push_logicitem_delete_temporary(logicitem_key_t logicitem_key)
    -> void {
    entries_.emplace_back(HistoryEntry::logicitem_delete_temporary);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_colliding_to_temporary(logicitem_key_t logicitem_key)
    -> void {
    entries_.emplace_back(HistoryEntry::logicitem_to_mode_temporary);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_temporary_to_colliding_expect_valid(
    logicitem_key_t logicitem_key) -> void {
    // optimize so mouse insertion does not produce endless entries
    if (get_back_vector(entries_) == HistoryEntry::logicitem_to_mode_temporary &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        pop_logicitem_to_mode_temporary();
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_to_mode_colliding_expect_valid);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_temporary_to_colliding_assume_colliding(
    logicitem_key_t logicitem_key) -> void {
    // optimize so mouse insertion does not produce endless entries
    if (get_back_vector(entries_) == HistoryEntry::logicitem_to_mode_temporary &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        pop_logicitem_to_mode_temporary();
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_to_mode_colliding_assume_colliding);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_colliding_to_insert(logicitem_key_t logicitem_key)
    -> void {
    // optimize so mouse resize does not produce endless entries
    if (get_back_vector(entries_) ==
            HistoryEntry::logicitem_to_mode_colliding_expect_valid &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        pop_logicitem_to_mode_colliding_expect_valid();
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_to_mode_insert);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_insert_to_colliding_expect_valid(
    logicitem_key_t logicitem_key) -> void {
    entries_.emplace_back(HistoryEntry::logicitem_to_mode_colliding_expect_valid);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_move_temporary(logicitem_key_t logicitem_key,
                                                 move_delta_t delta) -> void {
    entries_.emplace_back(HistoryEntry::logicitem_move_temporary);
    logicitem_keys_.emplace_back(logicitem_key);
    move_deltas_.emplace_back(delta);
}

auto HistoryStack::push_logicitem_change_attributes(
    logicitem_key_t logicitem_key, attributes_clock_generator_t&& attrs) -> void {
    // ignore even if in separate group, as GUI fires many
    if (last_non_group_entry(entries_) == HistoryEntry::logicitem_change_attributes &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_change_attributes);
    logicitem_keys_.emplace_back(logicitem_key);
    placed_logicitems_.emplace_back(PlacedLogicItem {
        .definition = LogicItemDefinition {.attrs_clock_generator = std::move(attrs)}});
}

auto HistoryStack::push_logicitem_add_visible_selection(logicitem_key_t logicitem_key)
    -> void {
    if (get_back_vector(entries_) == HistoryEntry::logicitem_remove_visible_selection &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        pop_logicitem_remove_visible_selection();
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_add_visible_selection);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::push_logicitem_remove_visible_selection(logicitem_key_t logicitem_key)
    -> void {
    if (get_back_vector(entries_) == HistoryEntry::logicitem_add_visible_selection &&
        at_back_vector(logicitem_keys_) == logicitem_key) {
        pop_logicitem_add_visible_selection();
        return;
    }

    entries_.emplace_back(HistoryEntry::logicitem_remove_visible_selection);
    logicitem_keys_.emplace_back(logicitem_key);
}

auto HistoryStack::pop_logicitem_create_temporary()
    -> std::pair<logicitem_key_t, PlacedLogicItem> {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_create_temporary);
    return {pop_back_vector(logicitem_keys_), pop_back_vector(placed_logicitems_)};
}

auto HistoryStack::pop_logicitem_delete_temporary() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_delete_temporary);
    return pop_back_vector(logicitem_keys_);
}

auto HistoryStack::pop_logicitem_to_mode_temporary() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_to_mode_temporary);
    return pop_back_vector(logicitem_keys_);
}

auto HistoryStack::pop_logicitem_to_mode_colliding_expect_valid() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::logicitem_to_mode_colliding_expect_valid);
    return pop_back_vector(logicitem_keys_);
}

auto HistoryStack::pop_logicitem_to_mode_colliding_assume_colliding() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::logicitem_to_mode_colliding_assume_colliding);
    return pop_back_vector(logicitem_keys_);
}

auto HistoryStack::pop_logicitem_to_mode_insert() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_to_mode_insert);
    return pop_back_vector(logicitem_keys_);
}

auto HistoryStack::pop_logicitem_move_temporary()
    -> std::pair<logicitem_key_t, move_delta_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_move_temporary);
    return {pop_back_vector(logicitem_keys_), pop_back_vector(move_deltas_)};
}

auto HistoryStack::pop_logicitem_change_attributes()
    -> std::pair<logicitem_key_t, attributes_clock_generator_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_change_attributes);
    return {pop_back_vector(logicitem_keys_),
            pop_back_vector(placed_logicitems_).definition.attrs_clock_generator.value()};
}

auto HistoryStack::pop_logicitem_add_visible_selection() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::logicitem_add_visible_selection);
    return pop_back_vector(logicitem_keys_);
}

auto HistoryStack::pop_logicitem_remove_visible_selection() -> logicitem_key_t {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::logicitem_remove_visible_selection);
    return pop_back_vector(logicitem_keys_);
}

//
// Decoration
//

auto HistoryStack::push_decoration_create_temporary(
    decoration_key_t decoration_key, PlacedDecoration&& placed_decoration) -> void {
    // optimize so mouse insertion does not produce endless entries
    if (get_back_vector(entries_) == HistoryEntry::decoration_delete_temporary &&
        at_back_vector(decoration_keys_) == decoration_key) {
        pop_decoration_delete_temporary();
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_create_temporary);
    decoration_keys_.emplace_back(decoration_key);
    placed_decorations_.emplace_back(std::move(placed_decoration));
}

auto HistoryStack::push_decoration_delete_temporary(decoration_key_t decoration_key)
    -> void {
    entries_.emplace_back(HistoryEntry::decoration_delete_temporary);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_colliding_to_temporary(decoration_key_t decoration_key)
    -> void {
    entries_.emplace_back(HistoryEntry::decoration_to_mode_temporary);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_temporary_to_colliding_expect_valid(
    decoration_key_t decoration_key) -> void {
    // optimize so mouse insertion does not produce endless entries
    if (get_back_vector(entries_) == HistoryEntry::decoration_to_mode_temporary &&
        at_back_vector(decoration_keys_) == decoration_key) {
        pop_decoration_to_mode_temporary();
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_to_mode_colliding_expect_valid);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_temporary_to_colliding_assume_colliding(
    decoration_key_t decoration_key) -> void {
    // optimize so mouse insertion does not produce endless entries
    if (get_back_vector(entries_) == HistoryEntry::decoration_to_mode_temporary &&
        at_back_vector(decoration_keys_) == decoration_key) {
        pop_decoration_to_mode_temporary();
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_to_mode_colliding_assume_colliding);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_colliding_to_insert(decoration_key_t decoration_key)
    -> void {
    // optimize so mouse resize does not produce endless entries
    if (get_back_vector(entries_) ==
            HistoryEntry::decoration_to_mode_colliding_expect_valid &&
        at_back_vector(decoration_keys_) == decoration_key) {
        pop_decoration_to_mode_colliding_expect_valid();
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_to_mode_insert);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_insert_to_colliding_expect_valid(
    decoration_key_t decoration_key) -> void {
    entries_.emplace_back(HistoryEntry::decoration_to_mode_colliding_expect_valid);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_move_temporary(decoration_key_t decoration_key,
                                                  move_delta_t delta) -> void {
    entries_.emplace_back(HistoryEntry::decoration_move_temporary);
    decoration_keys_.emplace_back(decoration_key);
    move_deltas_.emplace_back(delta);
}

auto HistoryStack::push_decoration_change_attributes(
    decoration_key_t decoration_key, attributes_text_element_t&& attrs) -> void {
    // ignore even if in separate group, as GUI fires many
    if (last_non_group_entry(entries_) == HistoryEntry::decoration_change_attributes &&
        at_back_vector(decoration_keys_) == decoration_key) {
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_change_attributes);
    decoration_keys_.emplace_back(decoration_key);
    placed_decorations_.emplace_back(PlacedDecoration {
        .definition = DecorationDefinition {.attrs_text_element = std::move(attrs)}});
}

auto HistoryStack::push_decoration_add_visible_selection(decoration_key_t decoration_key)
    -> void {
    if (get_back_vector(entries_) == HistoryEntry::decoration_remove_visible_selection &&
        at_back_vector(decoration_keys_) == decoration_key) {
        pop_decoration_remove_visible_selection();
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_add_visible_selection);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::push_decoration_remove_visible_selection(
    decoration_key_t decoration_key) -> void {
    if (get_back_vector(entries_) == HistoryEntry::decoration_add_visible_selection &&
        at_back_vector(decoration_keys_) == decoration_key) {
        pop_decoration_add_visible_selection();
        return;
    }

    entries_.emplace_back(HistoryEntry::decoration_remove_visible_selection);
    decoration_keys_.emplace_back(decoration_key);
}

auto HistoryStack::pop_decoration_create_temporary()
    -> std::pair<decoration_key_t, PlacedDecoration> {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_create_temporary);
    return {pop_back_vector(decoration_keys_), pop_back_vector(placed_decorations_)};
}

auto HistoryStack::pop_decoration_delete_temporary() -> decoration_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_delete_temporary);
    return pop_back_vector(decoration_keys_);
}

auto HistoryStack::pop_decoration_to_mode_temporary() -> decoration_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_to_mode_temporary);
    return pop_back_vector(decoration_keys_);
}

auto HistoryStack::pop_decoration_to_mode_colliding_expect_valid() -> decoration_key_t {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::decoration_to_mode_colliding_expect_valid);
    return pop_back_vector(decoration_keys_);
}

auto HistoryStack::pop_decoration_to_mode_colliding_assume_colliding()
    -> decoration_key_t {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::decoration_to_mode_colliding_assume_colliding);
    return pop_back_vector(decoration_keys_);
}

auto HistoryStack::pop_decoration_to_mode_insert() -> decoration_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_to_mode_insert);
    return pop_back_vector(decoration_keys_);
}

auto HistoryStack::pop_decoration_move_temporary()
    -> std::pair<decoration_key_t, move_delta_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_move_temporary);
    return {pop_back_vector(decoration_keys_), pop_back_vector(move_deltas_)};
}

auto HistoryStack::pop_decoration_change_attributes()
    -> std::pair<decoration_key_t, attributes_text_element_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_change_attributes);
    return {pop_back_vector(decoration_keys_),
            pop_back_vector(placed_decorations_).definition.attrs_text_element.value()};
}

auto HistoryStack::pop_decoration_add_visible_selection() -> decoration_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::decoration_add_visible_selection);
    return pop_back_vector(decoration_keys_);
}

auto HistoryStack::pop_decoration_remove_visible_selection() -> decoration_key_t {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::decoration_remove_visible_selection);
    return pop_back_vector(decoration_keys_);
}

//
// Segment
//

auto HistoryStack::push_segment_create_temporary(segment_key_t segment_key,
                                                 ordered_line_t line) -> void {
    entries_.emplace_back(HistoryEntry::segment_create_temporary);
    segment_keys_.emplace_back(segment_key);
    lines_.emplace_back(line);
}

auto HistoryStack::push_segment_delete_temporary(segment_key_t segment_key) -> void {
    entries_.emplace_back(HistoryEntry::segment_delete_temporary);
    segment_keys_.emplace_back(segment_key);
}

auto HistoryStack::push_segment_move_temporary(segment_key_t segment_key,
                                               move_delta_t delta) -> void {
    entries_.emplace_back(HistoryEntry::segment_move_temporary);
    segment_keys_.emplace_back(segment_key);
    move_deltas_.emplace_back(delta);
}

auto HistoryStack::push_segment_colliding_to_temporary(segment_key_t segment_key,
                                                       part_t part) -> void {
    entries_.emplace_back(HistoryEntry::segment_to_mode_temporary);
    segment_keys_.emplace_back(segment_key);
    parts_.emplace_back(part);
}

auto HistoryStack::push_segment_temporary_to_colliding_expect_valid(
    segment_key_t segment_key, part_t part) -> void {
    entries_.emplace_back(HistoryEntry::segment_to_mode_colliding_expect_valid);
    segment_keys_.emplace_back(segment_key);
    parts_.emplace_back(part);
}

auto HistoryStack::push_segment_temporary_to_colliding_assume_colliding(
    segment_key_t segment_key, part_t part) -> void {
    entries_.emplace_back(HistoryEntry::segment_to_mode_colliding_assume_colliding);
    segment_keys_.emplace_back(segment_key);
    parts_.emplace_back(part);
}

auto HistoryStack::push_segment_colliding_to_insert(segment_key_t segment_key,
                                                    part_t part) -> void {
    entries_.emplace_back(HistoryEntry::segment_to_mode_insert);
    segment_keys_.emplace_back(segment_key);
    parts_.emplace_back(part);
}

auto HistoryStack::push_segment_insert_to_colliding_expect_valid(
    segment_key_t segment_key, part_t part) -> void {
    entries_.emplace_back(HistoryEntry::segment_to_mode_colliding_expect_valid);
    segment_keys_.emplace_back(segment_key);
    parts_.emplace_back(part);
}

auto HistoryStack::push_segment_set_endpoints(segment_key_t segment_key,
                                              endpoints_t endpoints) -> void {
    entries_.emplace_back(HistoryEntry::segment_set_endpoints);
    segment_keys_.emplace_back(segment_key);
    endpoints_.emplace_back(endpoints);
}

auto HistoryStack::push_segment_merge(segment_key_t segment_key_0,
                                      segment_key_t segment_key_1) -> void {
    entries_.emplace_back(HistoryEntry::segment_merge);
    segment_keys_.emplace_back(segment_key_0);
    segment_keys_.emplace_back(segment_key_1);
}

auto HistoryStack::push_segment_split(split_segment_key_t definition) -> void {
    entries_.emplace_back(HistoryEntry::segment_split);
    segment_keys_.emplace_back(definition.source);
    segment_keys_.emplace_back(definition.new_key);
    offsets_.emplace_back(definition.split_offset);
}

auto HistoryStack::push_segment_add_visible_selection(segment_key_t segment_key,
                                                      part_t part) -> void {
    entries_.emplace_back(HistoryEntry::segment_add_visible_selection);
    segment_keys_.emplace_back(segment_key);
    parts_.emplace_back(part);
}

auto HistoryStack::pop_segment_create_temporary()
    -> std::pair<segment_key_t, ordered_line_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_create_temporary);
    return {
        pop_back_vector(segment_keys_),
        pop_back_vector(lines_),
    };
}

auto HistoryStack::pop_segment_delete_temporary() -> segment_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_delete_temporary);
    return pop_back_vector(segment_keys_);
}

auto HistoryStack::pop_segment_move_temporary()
    -> std::pair<segment_key_t, move_delta_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_move_temporary);
    return {pop_back_vector(segment_keys_), pop_back_vector(move_deltas_)};
}

auto HistoryStack::pop_segment_to_mode_temporary() -> std::pair<segment_key_t, part_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_to_mode_temporary);
    return {pop_back_vector(segment_keys_), pop_back_vector(parts_)};
}

auto HistoryStack::pop_segment_to_mode_colliding_expect_valid()
    -> std::pair<segment_key_t, part_t> {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::segment_to_mode_colliding_expect_valid);
    return {pop_back_vector(segment_keys_), pop_back_vector(parts_)};
}

auto HistoryStack::pop_segment_to_mode_colliding_assume_colliding()
    -> std::pair<segment_key_t, part_t> {
    Expects(pop_back_vector(entries_) ==
            HistoryEntry::segment_to_mode_colliding_assume_colliding);
    return {pop_back_vector(segment_keys_), pop_back_vector(parts_)};
}

auto HistoryStack::pop_segment_to_mode_insert() -> std::pair<segment_key_t, part_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_to_mode_insert);
    return {pop_back_vector(segment_keys_), pop_back_vector(parts_)};
}

auto HistoryStack::pop_segment_set_endpoints() -> std::pair<segment_key_t, endpoints_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_set_endpoints);
    return {pop_back_vector(segment_keys_), pop_back_vector(endpoints_)};
}

auto HistoryStack::pop_segment_merge() -> std::pair<segment_key_t, segment_key_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_merge);
    // separate pop to guarantee order
    const auto segment_key_1 = pop_back_vector(segment_keys_);
    const auto segment_key_0 = pop_back_vector(segment_keys_);
    return {segment_key_0, segment_key_1};
}

auto HistoryStack::pop_segment_split() -> split_segment_key_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_split);
    const auto new_segment = pop_back_vector(segment_keys_);
    const auto source = pop_back_vector(segment_keys_);
    return split_segment_key_t {
        .source = source,
        .new_key = new_segment,
        .split_offset = pop_back_vector(offsets_),
    };
}

auto HistoryStack::pop_segment_add_visible_selection()
    -> std::pair<segment_key_t, part_t> {
    Expects(pop_back_vector(entries_) == HistoryEntry::segment_add_visible_selection);
    return {pop_back_vector(segment_keys_), pop_back_vector(parts_)};
}

//
// Visible Selection
//

auto HistoryStack::push_visible_selection_clear() -> void {
    entries_.emplace_back(HistoryEntry::visible_selection_clear);
}

auto HistoryStack::push_visible_selection_set(StableSelection&& stable_selection)
    -> void {
    entries_.emplace_back(HistoryEntry::visible_selection_set);
    selections_.emplace_back(std::move(stable_selection));
}

auto HistoryStack::push_visible_selection_add_operation(
    const VisibleSelection::operation_t& operation) -> void {
    if (get_entry_before_skip(entries_, HistoryEntry::visible_selection_update_last) ==
        HistoryEntry::visible_selection_pop_last) {
        while (at_back_vector(entries_) == HistoryEntry::visible_selection_update_last) {
            pop_visible_selection_update_last();
        }
        pop_visible_selection_pop_last();
        return;
    }

    entries_.emplace_back(HistoryEntry::visible_selection_add_operation);
    selection_functions_.emplace_back(operation.function);
    selection_rects_.emplace_back(operation.rect);
}

auto HistoryStack::push_visible_selection_update_last(const rect_fine_t& rect) -> void {
    if (get_back_vector(entries_) == HistoryEntry::visible_selection_update_last) {
        return;
    }

    entries_.emplace_back(HistoryEntry::visible_selection_update_last);
    selection_rects_.emplace_back(rect);
}

auto HistoryStack::push_visible_selection_pop_last() -> void {
    entries_.emplace_back(HistoryEntry::visible_selection_pop_last);
}

auto HistoryStack::push_visible_selection_select_all() -> void {
    entries_.emplace_back(HistoryEntry::visible_selection_select_all);
}

auto HistoryStack::pop_visible_selection_clear() -> void {
    Expects(pop_back_vector(entries_) == HistoryEntry::visible_selection_clear);
}

auto HistoryStack::pop_visible_selection_set() -> StableSelection {
    Expects(pop_back_vector(entries_) == HistoryEntry::visible_selection_set);
    return pop_back_vector(selections_);
}

auto HistoryStack::pop_visible_selection_add_operation()
    -> visible_selection::operation_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::visible_selection_add_operation);
    return visible_selection::operation_t {
        .function = pop_back_vector(selection_functions_),
        .rect = pop_back_vector(selection_rects_),
    };
}

auto HistoryStack::pop_visible_selection_update_last() -> rect_fine_t {
    Expects(pop_back_vector(entries_) == HistoryEntry::visible_selection_update_last);
    return pop_back_vector(selection_rects_);
}

auto HistoryStack::pop_visible_selection_pop_last() -> void {
    Expects(pop_back_vector(entries_) == HistoryEntry::visible_selection_pop_last);
}

auto HistoryStack::pop_visible_selection_select_all() -> void {
    Expects(pop_back_vector(entries_) == HistoryEntry::visible_selection_select_all);
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

auto has_ungrouped_entries(const HistoryStack& stack) -> bool {
    const auto top_entry = stack.top_entry();

    return top_entry != std::nullopt && top_entry != HistoryEntry::new_group;
}

auto reopen_group(HistoryStack& stack) -> void {
    while (stack.top_entry() == HistoryEntry::new_group) {
        stack.pop_new_group();
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
