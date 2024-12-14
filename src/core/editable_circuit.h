#ifndef LOGICSIM_EDITABLE_CIRCUIT_H
#define LOGICSIM_EDITABLE_CIRCUIT_H

#include "core/component/editable_circuit/modifier.h"
#include "core/component/editable_circuit/selection_guard.h"
#include "core/format/struct.h"
#include "core/random/generator.h"
#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/line_insertion_type.h"
#include "core/vocabulary/placed_element.h"

namespace logicsim {

struct PlacedLogicItem;
struct PlacedDecoration;
struct CircuitDataAllocInfo;

class Layout;

class EditableCircuit {
   public:
    using Config = editable_circuit::ModifierConfig;

   public:
    [[nodiscard]] explicit EditableCircuit() = default;
    [[nodiscard]] explicit EditableCircuit(Layout&& layout, Config config = {});

    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto allocation_info() const -> CircuitDataAllocInfo;
    [[nodiscard]] auto operator==(const EditableCircuit&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto config() const -> Config;
    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto extract_layout() -> Layout;
    [[nodiscard]] auto modifier() const -> const editable_circuit::Modifier&;

    //
    // Undo & Redo
    //

    auto enable_history() -> void;
    auto disable_history() -> void;

    auto undo_group() -> void;
    auto redo_group() -> void;

    auto finish_undo_group() -> void;
    auto reopen_undo_group() -> void;

    [[nodiscard]] auto is_history_enabled() const -> bool;
    [[nodiscard]] auto has_undo() const -> bool;
    [[nodiscard]] auto has_redo() const -> bool;
    [[nodiscard]] auto has_ungrouped_undo_entries() const -> bool;
    [[nodiscard]] auto undo_groups_count() const -> std::size_t;

    //
    // Elements
    //

    auto add_logicitem(LogicItemDefinition&& definition, point_t position,
                       InsertionMode insertion_mode,
                       selection_id_t selection_id = null_selection_id) -> void;

    auto add_wire_segment(ordered_line_t line, InsertionMode insertion_mode,
                          selection_id_t selection_id = null_selection_id)
        -> segment_part_t;

    auto add_decoration(DecorationDefinition&& definition, point_t position,
                        InsertionMode insertion_mode,
                        selection_id_t selection_id = null_selection_id) -> void;

    /**
     * @brief: Changes insertion mode of the selection.
     *
     * Note that when segments are uninserted they need to be sanitized.
     *
     * Throws, if unsanitized segments are uninserted.
     */
    auto change_insertion_mode(selection_id_t selection_id,
                               InsertionMode new_insertion_mode) -> void;
    auto change_insertion_mode(Selection selection,
                               InsertionMode new_insertion_mode) -> void;

    auto move_or_delete_temporary(selection_id_t selection_id,
                                  move_delta_t delta) -> void;
    auto move_or_delete_temporary(Selection selection, move_delta_t delta) -> void;

    auto move_temporary_unchecked(const Selection& selection, move_delta_t delta) -> void;

    auto delete_all(selection_id_t selection_id) -> void;
    auto delete_all(Selection selection) -> void;

    //
    // Attributes
    //

    auto toggle_inverter(point_t point) -> void;
    auto toggle_wire_crosspoint(point_t point) -> void;
    auto set_attributes(logicitem_id_t logicitem_id,
                        attributes_clock_generator_t attrs) -> void;
    auto set_attributes(decoration_id_t decoration_id,
                        attributes_text_element_t attrs) -> void;

    //
    // Wire Regularization
    //

    /**
     * @brief: Regulaizes temporary segments so no artefacts arise from their history.
     *
     * When uninseting only a view segments from existing tree, the resulting
     * selection might contain split segments which are not necessary.
     * This would result in unexpected split points when reinserting the selection
     * later on.
     *
     * The regularization shall be applies to a selection of wires that is moved
     * together to a new location.
     */
    auto regularize_temporary_selection(
        const Selection& selection,
        std::optional<std::vector<point_t>> true_cross_points = {})
        -> std::vector<point_t>;

    /**
     * @brief: Return points where temporary wires shall be split before insertion.
     *
     * Without splits wires would be colliding with endpoints of inserted wires.
     *
     * Throws, if any segment in the selection is not temporary.
     * Throws, if any segment is partially selected.
     */
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

    auto add_to_selection(selection_id_t selection_id,
                          logicitem_id_t logicitem_id) -> void;
    auto add_to_selection(selection_id_t selection_id,
                          decoration_id_t decoration_id) -> void;
    auto add_to_selection(selection_id_t selection_id,
                          segment_part_t segment_part) -> void;
    auto remove_from_selection(selection_id_t selection_id,
                               logicitem_id_t logicitem_id) -> void;
    auto remove_from_selection(selection_id_t selection_id,
                               decoration_id_t decoration_id) -> void;
    auto remove_from_selection(selection_id_t selection_id,
                               segment_part_t segment_part) -> void;

    //
    // Visible Selections
    //

    auto clear_visible_selection() -> void;
    auto set_visible_selection(Selection selection) -> void;

    auto visible_selection_operation_count() const -> std::size_t;
    auto add_visible_selection_rect(SelectionFunction function, rect_fine_t rect) -> void;
    auto try_pop_last_visible_selection_rect() -> bool;
    auto try_update_last_visible_selection_rect(rect_fine_t rect) -> bool;
    auto apply_all_visible_selection_operations() -> void;

    [[nodiscard]] auto visible_selection() const -> const Selection&;
    [[nodiscard]] auto visible_selection_empty() const -> bool;

   private:
    editable_circuit::Modifier modifier_ {};
};

static_assert(std::regular<EditableCircuit>);

//
// Selection Guard
//

using SelectionGuard = editable_circuit::SelectionGuardTemplate<EditableCircuit>;

//
// Free Methods
//
/**
 * @brief: Check the class invariants manually, e.g. for tests.
 *
 * Checking the invariants is extremely expensive compared and needs to be
 * enabled with DEBUG_CHECK_CLASS_INVARIANTS on a per method level.
 */
[[nodiscard]] auto is_valid(const EditableCircuit& editable_circuit) -> bool;

auto add_placed_logicitem(EditableCircuit& editable_circuit,
                          PlacedLogicItem&& placed_logicitem,
                          InsertionMode insertion_mode,
                          selection_id_t selection_id = null_selection_id) -> void;
auto add_placed_decoration(EditableCircuit& editable_circuit,
                           PlacedDecoration&& placed_decoration,
                           InsertionMode insertion_mode,
                           selection_id_t selection_id = null_selection_id) -> void;
auto add_placed_element(EditableCircuit& editable_circuit, PlacedElement&& placed_element,
                        InsertionMode insertion_mode,
                        selection_id_t selection_id = null_selection_id) -> void;

auto add_wire_segments(EditableCircuit& editable_circuit, point_t p0, point_t p1,
                       LineInsertionType segment_type, InsertionMode insertion_mode,
                       selection_id_t selection_id = null_selection_id) -> void;

auto add_example(Rng& rng, EditableCircuit& editable_circuit) -> void;

[[nodiscard]] auto new_positions_representable(const EditableCircuit& editable_circuit,
                                               const Selection& selection,
                                               move_delta_t delta) -> bool;

/**
 * @brief: Return a list of cross points (3 or 4 wires ending) of the selection.
 */
[[nodiscard]] auto get_inserted_cross_points(const EditableCircuit& editable_circuit,
                                             const Selection& selection)
    -> std::vector<point_t>;

auto save_delete_all(EditableCircuit& editable_circuit,
                     selection_id_t selection_id) -> void;

auto save_destroy_selection(EditableCircuit& editable_circuit,
                            selection_id_t selection_id) -> void;

auto visible_selection_select_all(EditableCircuit& editable_circuit) -> void;

auto visible_selection_delete_all(EditableCircuit& editable_circuit) -> void;

[[nodiscard]] auto visible_selection_anything_colliding(EditableCircuit& editable_circuit)
    -> bool;

[[nodiscard]] auto get_single_logicitem(const EditableCircuit& editable_circuit,
                                        selection_id_t selection_id) -> logicitem_id_t;
[[nodiscard]] auto get_single_decoration(const EditableCircuit& editable_circuit,
                                         selection_id_t selection_id) -> decoration_id_t;

}  // namespace logicsim

#endif
