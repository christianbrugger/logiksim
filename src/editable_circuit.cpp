#include "editable_circuit.h"

#include "random/generator.h"
#include "random/wire.h"

#include <fmt/core.h>

namespace logicsim {

EditableCircuit::EditableCircuit(Layout&& layout__) : modifier_ {std::move(layout__)} {}

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
// Elements
//

auto EditableCircuit::add_logicitem(const LogicItemDefinition& definition,
                                    point_t position, InsertionMode insertion_mode,
                                    selection_id_t selection_id) -> void {
    const auto logicitem_id =
        modifier_.add_logicitem(definition, position, insertion_mode);

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

auto EditableCircuit::change_insertion_mode(selection_id_t selection_id,
                                            InsertionMode new_insertion_mode) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, selection_id);
    change_insertion_mode_consuming(modifier_, guard.selection_id(), new_insertion_mode);
}

auto EditableCircuit::change_insertion_mode(Selection selection__,
                                            InsertionMode new_insertion_mode) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, std::move(selection__));
    change_insertion_mode_consuming(modifier_, guard.selection_id(), new_insertion_mode);
}

auto EditableCircuit::move_or_delete_temporary(selection_id_t selection_id, int delta_x,
                                               int delta_y) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, selection_id);
    move_or_delete_temporary_consuming(modifier_, guard.selection_id(), delta_x, delta_y);
}

auto EditableCircuit::move_or_delete_temporary(Selection selection__, int delta_x,
                                               int delta_y) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, std::move(selection__));
    move_or_delete_temporary_consuming(modifier_, guard.selection_id(), delta_x, delta_y);
}

auto EditableCircuit::move_temporary_unchecked(const Selection& selection, int delta_x,
                                               int delta_y) -> void {
    editable_circuit::move_temporary_unchecked(modifier_, selection, delta_x, delta_y);
}

auto EditableCircuit::delete_all(selection_id_t selection_id) -> void {
    editable_circuit::delete_all(modifier_, selection_id);
}

auto EditableCircuit::delete_all(Selection selection__) -> void {
    using namespace editable_circuit;

    const auto guard = ModifierSelectionGuard(modifier_, std::move(selection__));
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
                                     attributes_clock_generator_t attrs__) -> void {
    modifier_.set_attributes(logicitem_id, std::move(attrs__));
}

//
// Wire Regularization
//

auto EditableCircuit::regularize_temporary_selection(
    const Selection& selection, std::optional<std::vector<point_t>> true_cross_points__)
    -> std::vector<point_t> {
    return modifier_.regularize_temporary_selection(selection,
                                                    std::move(true_cross_points__));
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

auto EditableCircuit::create_selection(Selection selection__) -> selection_id_t {
    return modifier_.create_selection(std::move(selection__));
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

auto EditableCircuit::set_selection(selection_id_t selection_id, Selection selection__)
    -> void {
    modifier_.set_selection(selection_id, std::move(selection__));
}

auto EditableCircuit::add_to_selection(selection_id_t selection_id,
                                       logicitem_id_t logicitem_id) -> void {
    modifier_.add_to_selection(selection_id, logicitem_id);
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
                                            segment_part_t segment_part) -> void {
    modifier_.add_to_selection(selection_id, segment_part);
}

//
// Visible Selections
//

auto EditableCircuit::clear_visible_selection() -> void {
    modifier_.clear_visible_selection();
}

auto EditableCircuit::set_visible_selection(Selection selection__) -> void {
    modifier_.set_visible_selection(std::move(selection__));
}

auto EditableCircuit::visible_selection_operation_count() const -> std::size_t {
    return modifier_.circuit_data().visible_selection.operation_count();
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
                                 const Selection& selection, int delta_x, int delta_y)
    -> bool {
    return editable_circuit::new_positions_representable(editable_circuit.layout(),
                                                         selection, delta_x, delta_y);
}

auto get_inserted_cross_points(const EditableCircuit& editable_circuit,
                               const Selection& selection) -> std::vector<point_t> {
    return editable_circuit::get_inserted_cross_points(editable_circuit.modifier(),
                                                       selection);
}

auto save_delete_all(EditableCircuit& editable_circuit, selection_id_t selection_id)
    -> void {
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
    const auto rect = rect_fine_t {point_fine_t {grid_t::min(), grid_t::min()},
                                   point_fine_t {grid_t::max(), grid_t::max()}};

    editable_circuit.clear_visible_selection();
    editable_circuit.add_visible_selection_rect(SelectionFunction::add, rect);
    // for optimization
    editable_circuit.apply_all_visible_selection_operations();
}

auto visible_selection_delete_all(EditableCircuit& editable_circuit) -> void {
    // Clear the visible selection before deleting for optimization.
    // So it is not tracked during deletion. (10% speedup)

    auto selection__ = Selection {editable_circuit.visible_selection()};
    editable_circuit.clear_visible_selection();
    editable_circuit.delete_all(std::move(selection__));
}

}  // namespace logicsim
