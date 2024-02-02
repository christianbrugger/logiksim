#ifndef LOGICSIM_EDITABLE_CIRCUIT2_H
#define LOGICSIM_EDITABLE_CIRCUIT2_H

#include "component/editable_circuit/layout_modifier.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/line_insertion_type.h"

namespace logicsim {

class EditableCircuit2 {
   public:
    [[nodiscard]] explicit EditableCircuit2() = default;
    [[nodiscard]] explicit EditableCircuit2(Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;

    //
    // Sub-components
    //

    [[nodiscard]] auto layout() const -> const Layout&;
    // TODO move LayoutIndex to editable_circuit namespace ?
    [[nodiscard]] auto index() const -> const LayoutIndex&;
    [[nodiscard]] auto selections() const -> const editable_circuit::SelectionStore&;
    // TODO move VisibleSelection to editable_circuit namespace ?
    [[nodiscard]] auto visible_selection() const -> const VisibleSelection&;

    //
    // Logic Items
    //

    auto add_logic_item(const LogicItemDefinition& definition, point_t position,
                        InsertionMode insertion_mode,
                        selection_id_t selection_id = null_selection_id) -> void;

    auto add_line_segments(point_t p0, point_t p1, LineInsertionType segment_type,
                           InsertionMode insertion_mode,
                           selection_id_t selection_id = null_selection_id) -> void;

    auto change_insertion_mode(selection_id_t selection_id,
                               InsertionMode new_insertion_mode) -> void;

    auto move_or_delete_temporary(selection_id_t selection_id, int delta_x, int delta_y)
        -> void;

    auto move_temporary_unchecked(const Selection& selection, int delta_x, int delta_y)
        -> void;

    auto delete_all(selection_id_t selection_id) -> void;

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

    auto split_before_insert(const Selection& selection) -> void;

    //
    // Selections
    //

    //
    // Visible Selections
    //

   private:
    editable_circuit::LayoutModifier modifier_ {};
};

auto add_example(EditableCircuit2& editable_circuit) -> void;

[[nodiscard]] auto new_positions_representable(const EditableCircuit2& editable_circuit,
                                               const Selection& selection, int delta_x,
                                               int delta_y) -> bool;

auto change_insertion_mode(EditableCircuit2& editable_circuit, Selection selection,
                           InsertionMode new_insertion_mode) -> void;

auto move_or_delete_temporary(EditableCircuit2& editable_circuit, Selection selection,
                              int delta_x, int delta_y) -> void;

auto delete_all(EditableCircuit2& editable_circuit, Selection selection) -> void;

auto capture_inserted_cross_points(const EditableCircuit2& editable_circuit,
                                   const Selection& selection) -> std::vector<point_t>;

}  // namespace logicsim

#endif
