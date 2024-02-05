#ifndef LOGICSIM_EDITABLE_CIRCUIT_H
#define LOGICSIM_EDITABLE_CIRCUIT_H

#include "component/editable_circuit/modifier.h"
#include "component/editable_circuit/selection_guard.h"
#include "format/struct.h"
#include "random/generator.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/line_insertion_type.h"

namespace logicsim {

class Layout;

class EditableCircuit {
   public:
    [[nodiscard]] explicit EditableCircuit() = default;
    [[nodiscard]] explicit EditableCircuit(Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto extract_layout() -> Layout;
    [[nodiscard]] auto modifier() const -> const editable_circuit::Modifier&;

    //
    // Elements
    //

    auto add_logicitem(const LogicItemDefinition& definition, point_t position,
                       InsertionMode insertion_mode,
                       selection_id_t selection_id = null_selection_id) -> void;

    auto add_wire_segment(ordered_line_t line, InsertionMode insertion_mode,
                          selection_id_t selection_id = null_selection_id)
        -> segment_part_t;

    auto change_insertion_mode(selection_id_t selection_id,
                               InsertionMode new_insertion_mode) -> void;
    auto change_insertion_mode(Selection selection, InsertionMode new_insertion_mode)
        -> void;

    auto move_or_delete_temporary(selection_id_t selection_id, int delta_x, int delta_y)
        -> void;
    auto move_or_delete_temporary(Selection selection, int delta_x, int delta_y) -> void;

    auto move_temporary_unchecked(const Selection& selection, int delta_x, int delta_y)
        -> void;

    auto delete_all(selection_id_t selection_id) -> void;
    auto delete_all(Selection selection) -> void;

    //
    // Attributes
    //

    auto toggle_inverter(point_t point) -> void;
    auto toggle_wire_crosspoint(point_t point) -> void;
    auto set_attributes(logicitem_id_t logicitem_id, attributes_clock_generator_t attrs)
        -> void;

    //
    // Wire Regularization
    //

    auto regularize_temporary_selection(
        const Selection& selection,
        std::optional<std::vector<point_t>> true_cross_points = {})
        -> std::vector<point_t>;

    auto split_temporary_before_insert(selection_id_t selection_id) -> void;
    auto split_temporary_before_insert(const Selection& selection) -> void;

    //
    // Layout Index
    //

    using query_entry = spatial_index::tree_payload_t;

    // TODO rename
    [[nodiscard]] auto query_selection(rect_fine_t rect) const
        -> std::vector<query_entry>;

    // TODO rename
    [[nodiscard]] auto has_element(point_fine_t point) const -> bool;

    //
    // Selections
    //

    [[nodiscard]] auto create_selection() -> selection_id_t;
    [[nodiscard]] auto create_selection(Selection selection) -> selection_id_t;
    [[nodiscard]] auto create_selection(selection_id_t copy_id) -> selection_id_t;
    auto destroy_selection(selection_id_t selection_id) -> void;

    [[nodiscard]] auto selection_count() const -> std::size_t;
    [[nodiscard]] auto selection_exists(selection_id_t selection_id) const -> bool;
    [[nodiscard]] auto selection(selection_id_t selection_id) const -> const Selection&;
    auto set_selection(selection_id_t selection_id, Selection selection) -> void;

    auto add_to_selection(selection_id_t, logicitem_id_t logicitem_id) -> void;
    auto add_to_selection(selection_id_t, segment_part_t segment_part) -> void;
    auto remove_from_selection(selection_id_t, logicitem_id_t logicitem_id) -> void;
    auto remove_from_selection(selection_id_t, segment_part_t segment_part) -> void;

    //
    // Visible Selections
    //

    auto clear_visible_selection() -> void;
    auto set_visible_selection(Selection selection) -> void;

    auto add_visible_selection_rect(SelectionFunction function, rect_fine_t rect) -> void;
    auto try_pop_last_visible_selection_rect() -> bool;
    auto try_update_last_visible_selection_rect(rect_fine_t rect) -> bool;
    auto apply_all_visible_selection_operations() -> void;

    [[nodiscard]] auto visible_selection() const -> const Selection&;
    [[nodiscard]] auto visible_selection_empty() const -> bool;

   private:
    editable_circuit::Modifier modifier_ {};
};

//
// Selection Guard
//

using SelectionGuard = editable_circuit::SelectionGuardTemplate<EditableCircuit>;

//
// Free Methods
//

auto add_wire_segments(EditableCircuit& editable_circuit, point_t p0, point_t p1,
                       LineInsertionType segment_type, InsertionMode insertion_mode,
                       selection_id_t selection_id = null_selection_id) -> void;

auto add_example(Rng& rng, EditableCircuit& editable_circuit) -> void;

[[nodiscard]] auto new_positions_representable(const EditableCircuit& editable_circuit,
                                               const Selection& selection, int delta_x,
                                               int delta_y) -> bool;

[[nodiscard]] auto get_inserted_cross_points(const EditableCircuit& editable_circuit,
                                             const Selection& selection)
    -> std::vector<point_t>;

auto save_delete_all(EditableCircuit& editable_circuit, selection_id_t selection_id)
    -> void;

auto save_destroy_selection(EditableCircuit& editable_circuit,
                            selection_id_t selection_id) -> void;

auto visible_selection_select_all(EditableCircuit& editable_circuit) -> void;

auto visible_selection_delete_all(EditableCircuit& editable_circuit) -> void;

}  // namespace logicsim

#endif
