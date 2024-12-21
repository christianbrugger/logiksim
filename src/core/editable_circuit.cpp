#include "core/editable_circuit.h"

#include "core/algorithm/overload.h"
#include "core/random/generator.h"
#include "core/random/wire.h"
#include "core/vocabulary/allocation_info.h"
#include "core/vocabulary/placed_decoration.h"
#include "core/vocabulary/placed_logicitem.h"

#include <fmt/core.h>

namespace logicsim {

EditableCircuit::EditableCircuit(Layout&& layout, Config config)
    : modifier_ {std::move(layout), config} {}

auto EditableCircuit::allocated_size() const -> std::size_t {
    return modifier_.circuit_data().allocated_size();
}

auto EditableCircuit::allocation_info() const -> CircuitDataAllocInfo {
    return modifier_.circuit_data().allocation_info();
}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}}}", modifier_.circuit_data().layout);
}

auto EditableCircuit::layout() const -> const Layout& {
    return modifier_.circuit_data().layout;
}

auto EditableCircuit::extract_layout() -> Layout {
    return modifier_.extract_layout();
}

auto EditableCircuit::modifier() const -> const editable_circuit::Modifier& {
    return modifier_;
}

//
// Undo & Redo
//

auto EditableCircuit::enable_history() -> void {
    modifier_.enable_history();
}

auto EditableCircuit::disable_history() -> void {
    modifier_.disable_history();
}

auto EditableCircuit::undo_group() -> void {
    modifier_.undo_group();
}

auto EditableCircuit::redo_group() -> void {
    modifier_.redo_group();
}

auto EditableCircuit::clear_undo_history() -> void {
    modifier_.clear_undo_history();
}

auto EditableCircuit::clear_redo_history() -> void {
    modifier_.clear_redo_history();
}

auto EditableCircuit::finish_undo_group() -> void {
    modifier_.finish_undo_group();
}

auto EditableCircuit::reopen_undo_group() -> void {
    modifier_.reopen_undo_group();
}

//
// Elements
//

auto EditableCircuit::add_logicitem(LogicItemDefinition&& definition, point_t position,
                                    InsertionMode insertion_mode,
                                    selection_id_t selection_id) -> void {
    const auto logicitem_id =
        modifier_.add_logicitem(std::move(definition), position, insertion_mode);

    if (selection_id && logicitem_id) {
        modifier_.add_to_selection(selection_id, logicitem_id);
    }
}

auto EditableCircuit::add_wire_segment(ordered_line_t line, InsertionMode insertion_mode,
                                       selection_id_t selection_id) -> segment_part_t {
    const auto segment_part = modifier_.add_wire_segment(line, insertion_mode);

    if (selection_id && segment_part) {
        modifier_.add_to_selection(selection_id, segment_part);
    }

    return segment_part;
}

auto EditableCircuit::add_decoration(DecorationDefinition&& definition, point_t position,
                                     InsertionMode insertion_mode,
                                     selection_id_t selection_id) -> void {
    const auto decoration_id =
        modifier_.add_decoration(std::move(definition), position, insertion_mode);

    if (selection_id && decoration_id) {
        modifier_.add_to_selection(selection_id, decoration_id);
    }
}

auto EditableCircuit::change_insertion_mode(selection_id_t selection_id,
                                            InsertionMode new_insertion_mode) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard {modifier_, selection_id};
    change_insertion_mode_consuming(modifier_, guard.selection_id(), new_insertion_mode);
}

auto EditableCircuit::change_insertion_mode(Selection selection,
                                            InsertionMode new_insertion_mode) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard {modifier_, std::move(selection)};
    change_insertion_mode_consuming(modifier_, guard.selection_id(), new_insertion_mode);
}

auto EditableCircuit::move_or_delete_temporary(selection_id_t selection_id,
                                               move_delta_t delta) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard {modifier_, selection_id};
    move_or_delete_temporary_consuming(modifier_, guard.selection_id(), delta);
}

auto EditableCircuit::move_or_delete_temporary(Selection selection,
                                               move_delta_t delta) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard {modifier_, std::move(selection)};
    move_or_delete_temporary_consuming(modifier_, guard.selection_id(), delta);
}

auto EditableCircuit::move_temporary_unchecked(const Selection& selection,
                                               move_delta_t delta) -> void {
    editable_circuit::move_temporary_unchecked(modifier_, selection, delta);
}

auto EditableCircuit::delete_all(selection_id_t selection_id) -> void {
    editable_circuit::delete_all(modifier_, selection_id);
}

auto EditableCircuit::delete_all(Selection selection) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard {modifier_, std::move(selection)};
    editable_circuit::delete_all(modifier_, guard.selection_id());
}

//
// Attributes
//

auto EditableCircuit::toggle_inverter(point_t point) -> void {
    modifier_.toggle_inverter(point);
}

auto EditableCircuit::toggle_wire_crosspoint(point_t point) -> void {
    modifier_.toggle_wire_crosspoint(point);
}

auto EditableCircuit::set_attributes(logicitem_id_t logicitem_id,
                                     attributes_clock_generator_t attrs) -> void {
    modifier_.set_attributes(logicitem_id, std::move(attrs));
}

auto EditableCircuit::set_attributes(decoration_id_t decoration_id,
                                     attributes_text_element_t attrs) -> void {
    modifier_.set_attributes(decoration_id, std::move(attrs));
}

//
// Wire Regularization
//

auto EditableCircuit::regularize_temporary_selection(
    const Selection& selection,
    std::optional<std::vector<point_t>> true_cross_points) -> std::vector<point_t> {
    return modifier_.regularize_temporary_selection(selection,
                                                    std::move(true_cross_points));
}

auto EditableCircuit::split_temporary_before_insert(selection_id_t selection_id) -> void {
    return split_temporary_before_insert(selection(selection_id));
}

auto EditableCircuit::split_temporary_before_insert(const Selection& selection) -> void {
    using namespace editable_circuit;

    const auto points = get_temporary_selection_splitpoints(modifier_, selection);
    modifier_.split_temporary_segments(selection, points);
}

//
// Layout Index
//

auto EditableCircuit::query_selection(rect_fine_t rect) const
    -> std::vector<query_entry> {
    return modifier_.circuit_data().index.selection_index().query_selection(rect);
}

auto EditableCircuit::has_element(point_fine_t point) const -> bool {
    return modifier_.circuit_data().index.selection_index().has_element(point);
}

//
// Selections
//

auto EditableCircuit::create_selection() -> selection_id_t {
    return modifier_.create_selection();
}

auto EditableCircuit::create_selection(Selection selection) -> selection_id_t {
    return modifier_.create_selection(std::move(selection));
}

auto EditableCircuit::create_selection(selection_id_t copy_id) -> selection_id_t {
    return modifier_.create_selection(copy_id);
}

auto EditableCircuit::destroy_selection(selection_id_t selection_id) -> void {
    modifier_.destroy_selection(selection_id);
}

auto EditableCircuit::selection_count() const -> std::size_t {
    return modifier_.circuit_data().selection_store.size();
}

auto EditableCircuit::selection_exists(selection_id_t selection_id) const -> bool {
    return modifier_.circuit_data().selection_store.contains(selection_id);
}

auto EditableCircuit::selection(selection_id_t selection_id) const -> const Selection& {
    return modifier_.circuit_data().selection_store.at(selection_id);
}

auto EditableCircuit::set_selection(selection_id_t selection_id,
                                    Selection selection) -> void {
    modifier_.set_selection(selection_id, std::move(selection));
}

auto EditableCircuit::add_to_selection(selection_id_t selection_id,
                                       logicitem_id_t logicitem_id) -> void {
    modifier_.add_to_selection(selection_id, logicitem_id);
}

auto EditableCircuit::add_to_selection(selection_id_t selection_id,
                                       decoration_id_t decoration_id) -> void {
    modifier_.add_to_selection(selection_id, decoration_id);
}

auto EditableCircuit::add_to_selection(selection_id_t selection_id,
                                       segment_part_t segment_part) -> void {
    modifier_.add_to_selection(selection_id, segment_part);
}

auto EditableCircuit::remove_from_selection(selection_id_t selection_id,
                                            logicitem_id_t logicitem_id) -> void {
    modifier_.add_to_selection(selection_id, logicitem_id);
}

auto EditableCircuit::remove_from_selection(selection_id_t selection_id,
                                            decoration_id_t decoration_id) -> void {
    modifier_.add_to_selection(selection_id, decoration_id);
}

auto EditableCircuit::remove_from_selection(selection_id_t selection_id,
                                            segment_part_t segment_part) -> void {
    modifier_.add_to_selection(selection_id, segment_part);
}

//
// Visible Selections
//

auto EditableCircuit::clear_visible_selection() -> void {
    modifier_.clear_visible_selection();
}

auto EditableCircuit::set_visible_selection(Selection selection) -> void {
    modifier_.set_visible_selection(std::move(selection));
}

auto EditableCircuit::visible_selection_operation_count() const -> std::size_t {
    return modifier_.circuit_data().visible_selection.operations().size();
}

auto EditableCircuit::add_visible_selection_rect(SelectionFunction function,
                                                 rect_fine_t rect) -> void {
    modifier_.add_visible_selection_rect(function, rect);
}

auto EditableCircuit::try_pop_last_visible_selection_rect() -> bool {
    return modifier_.try_pop_last_visible_selection_rect();
}

auto EditableCircuit::try_update_last_visible_selection_rect(rect_fine_t rect) -> bool {
    return modifier_.try_update_last_visible_selection_rect(rect);
}

auto EditableCircuit::apply_all_visible_selection_operations() -> void {
    modifier_.apply_all_visible_selection_operations();
}

auto EditableCircuit::visible_selection() const -> const Selection& {
    const auto& circuit = modifier_.circuit_data();

    return circuit.visible_selection.selection(circuit.layout, circuit.index);
}

auto EditableCircuit::visible_selection_empty() const -> bool {
    return modifier_.circuit_data().visible_selection.empty();
}

//
// Free Methods
//

auto is_valid(const EditableCircuit& editable_circuit) -> bool {
    return is_valid(editable_circuit.modifier());
}

auto get_config(const EditableCircuit& editable_circuit) -> EditableCircuit::Config {
    return editable_circuit::get_config(editable_circuit.modifier());
}

auto add_placed_logicitem(EditableCircuit& editable_circuit,
                          PlacedLogicItem&& placed_logicitem,
                          InsertionMode insertion_mode,
                          selection_id_t selection_id) -> void {
    editable_circuit.add_logicitem(std::move(placed_logicitem.definition),
                                   placed_logicitem.position, insertion_mode,
                                   selection_id);
}

auto add_placed_decoration(EditableCircuit& editable_circuit,
                           PlacedDecoration&& placed_decoration,
                           InsertionMode insertion_mode,
                           selection_id_t selection_id) -> void {
    editable_circuit.add_decoration(std::move(placed_decoration.definition),
                                    placed_decoration.position, insertion_mode,
                                    selection_id);
}

auto add_placed_element(EditableCircuit& editable_circuit, PlacedElement&& placed_element,
                        InsertionMode insertion_mode,
                        selection_id_t selection_id) -> void {
    std::visit(overload(
                   [&](PlacedLogicItem&& placed_logicitem) {
                       add_placed_logicitem(editable_circuit, std::move(placed_logicitem),
                                            insertion_mode, selection_id);
                   },
                   [&](PlacedDecoration&& placed_decoration) {
                       add_placed_decoration(editable_circuit,
                                             std::move(placed_decoration), insertion_mode,
                                             selection_id);
                   }),
               std::move(placed_element));
}

namespace {

auto _to_line(point_t p0, point_t p1) -> ordered_line_t {
    return ordered_line_t {line_t {p0, p1}};
}

}  // namespace

auto add_wire_segments(EditableCircuit& editable_circuit, point_t p0, point_t p1,
                       LineInsertionType segment_type, InsertionMode insertion_mode,
                       selection_id_t selection_id) -> void {
    const auto mode = insertion_mode;

    switch (segment_type) {
        using enum LineInsertionType;

        case horizontal_first: {
            const auto pm = point_t {p1.x, p0.y};
            if (p0.x != pm.x) {
                editable_circuit.add_wire_segment(_to_line(p0, pm), mode, selection_id);
            }
            if (pm.y != p1.y) {
                editable_circuit.add_wire_segment(_to_line(pm, p1), mode, selection_id);
            }
            break;
        }

        case vertical_first: {
            const auto pm = point_t {p0.x, p1.y};
            if (p0.y != pm.y) {
                editable_circuit.add_wire_segment(_to_line(p0, pm), mode, selection_id);
            }
            if (pm.x != p1.x) {
                editable_circuit.add_wire_segment(_to_line(pm, p1), mode, selection_id);
            }
            break;
        }
    }
}

auto add_example(Rng& rng, EditableCircuit& editable_circuit) -> void {
    add_many_wires_and_buttons(rng, editable_circuit);
}

auto new_positions_representable(const EditableCircuit& editable_circuit,
                                 const Selection& selection, move_delta_t delta) -> bool {
    return editable_circuit::new_positions_representable(editable_circuit.layout(),
                                                         selection, delta);
}

auto get_inserted_cross_points(const EditableCircuit& editable_circuit,
                               const Selection& selection) -> std::vector<point_t> {
    return editable_circuit::get_inserted_cross_points(editable_circuit.modifier(),
                                                       selection);
}

auto save_delete_all(EditableCircuit& editable_circuit,
                     selection_id_t selection_id) -> void {
    if (editable_circuit.selection_exists(selection_id)) {
        editable_circuit.delete_all(selection_id);
    }
}

auto save_destroy_selection(EditableCircuit& editable_circuit,
                            selection_id_t selection_id) -> void {
    if (editable_circuit.selection_exists(selection_id)) {
        editable_circuit.destroy_selection(selection_id);
    }
}

auto visible_selection_select_all(EditableCircuit& editable_circuit) -> void {
    editable_circuit.set_visible_selection(select_all(editable_circuit.layout()));
}

auto visible_selection_delete_all(EditableCircuit& editable_circuit) -> void {
    // Clear the visible selection before deleting for optimization.
    // So it is not tracked during deletion. (10% speedup)

    auto selection = Selection {editable_circuit.visible_selection()};
    editable_circuit.clear_visible_selection();
    editable_circuit.delete_all(std::move(selection));
}

[[nodiscard]] auto visible_selection_anything_colliding(EditableCircuit& editable_circuit)
    -> bool {
    return anything_colliding(editable_circuit.visible_selection(),
                              editable_circuit.layout());
}

auto get_single_logicitem(const EditableCircuit& editable_circuit,
                          selection_id_t selection_id) -> logicitem_id_t {
    if (editable_circuit.selection_exists(selection_id)) {
        return get_single_logicitem(editable_circuit.selection(selection_id));
    }
    return null_logicitem_id;
}

auto get_single_decoration(const EditableCircuit& editable_circuit,
                           selection_id_t selection_id) -> decoration_id_t {
    if (editable_circuit.selection_exists(selection_id)) {
        return get_single_decoration(editable_circuit.selection(selection_id));
    }
    return null_decoration_id;
}

//
// History
//

auto is_history_enabled(const EditableCircuit& editable_circuit) -> bool {
    return editable_circuit::is_history_enabled(editable_circuit.modifier());
}

auto has_undo(const EditableCircuit& editable_circuit) -> bool {
    return editable_circuit::has_undo(editable_circuit.modifier());
}

auto has_redo(const EditableCircuit& editable_circuit) -> bool {
    return editable_circuit::has_redo(editable_circuit.modifier());
}

auto has_ungrouped_undo_entries(const EditableCircuit& editable_circuit) -> bool {
    return editable_circuit::has_ungrouped_undo_entries(editable_circuit.modifier());
}

auto undo_groups_count(const EditableCircuit& editable_circuit) -> std::size_t {
    return editable_circuit::undo_groups_count(editable_circuit.modifier());
}

auto last_non_group_undo_entry_is_attribute_change(
    const EditableCircuit& editable_circuit) -> bool {
    using enum editable_circuit::HistoryEntry;

    const auto entry = editable_circuit.modifier()
                           .circuit_data()
                           .history.undo_stack.top_non_group_entry();

    return entry == logicitem_change_attributes ||  //
           entry == decoration_change_attributes;
}

}  // namespace logicsim
