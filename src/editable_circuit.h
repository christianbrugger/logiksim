#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "component/editable_circuit/layout_index.h"
#include "component/editable_circuit/message_sender.h"
#include "component/editable_circuit/selection_store.h"
#include "component/editable_circuit/visible_selection.h"
#include "format/struct.h"
#include "layout.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/line_insertion_type.h"

#include <optional>
#include <span>

namespace logicsim {

struct point_t;
struct logicitem_id_t;
struct LogicItemDefinition;

namespace editable_circuit {
class MessageSender;
struct State;
}  // namespace editable_circuit

class EditableCircuit {
   public:
    [[nodiscard]] explicit EditableCircuit();
    [[nodiscard]] explicit EditableCircuit(Layout&& layout);

    [[nodiscard]] auto format() const -> std::string;

    // TODO remove
    auto validate() -> void;

    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto extract_layout() -> Layout;

    // adding
    auto add_example() -> void;

    // TODO remove duplicate methods, always require handle ?
    auto add_logic_item(const LogicItemDefinition& definition, point_t position,
                        InsertionMode insertion_mode,
                        selection_id_t selection_id = null_selection_id) -> void;
    auto add_line_segment(line_t line, InsertionMode insertion_mode,
                          selection_id_t selection_id = null_selection_id) -> void;

    auto add_line_segments(point_t p0, point_t p1, LineInsertionType segment_type,
                           InsertionMode insertion_mode,
                           selection_id_t selection_id = null_selection_id) -> void;

    // changing
    auto change_insertion_mode(selection_id_t selection_id,
                               InsertionMode new_insertion_mode) -> void;
    auto change_insertion_mode(Selection selection, InsertionMode new_insertion_mode)
        -> void;
    [[nodiscard]] auto new_positions_representable(const Selection& selection,
                                                   int delta_x, int delta_y) const
        -> bool;
    auto move_or_delete(selection_id_t selection_id, int delta_x, int delta_y) -> void;
    auto move_or_delete(Selection selection, int delta_x, int delta_y) -> void;
    /**
     * @brief: Efficiently moves all items in the selection.
     *
     * Pre-conditions:
     *   + all selected items are temporary
     *   + all new positions are representable
     *   + segment lines are all fully selected, not partial
     */
    auto move_unchecked(const Selection& selection, int delta_x, int delta_y) -> void;
    auto delete_all(selection_id_t selection_id) -> void;
    auto delete_all(Selection selection) -> void;

    auto toggle_inverter(point_t point) -> void;
    auto toggle_wire_crosspoint(point_t point) -> void;
    auto set_attributes(logicitem_id_t logicitem_id, attributes_clock_generator_t attrs)
        -> void;

    // Wire Mode Change Helpers

    // adds crosspoints, merges wire segments and returns splitpoints
    auto regularize_temporary_selection(
        const Selection& selection,
        std::optional<std::vector<point_t>> true_cross_points = {})
        -> std::vector<point_t>;
    // free method ?
    auto capture_inserted_cross_points(const Selection& selection) const
        -> std::vector<point_t>;
    // TODO find better name
    auto split_before_insert(selection_id_t selection_id) -> void;
    auto split_before_insert(const Selection& selection) -> void;

    // selections
    [[nodiscard]] auto selection_count() const -> std::size_t;
    /**
     * @brief: Return reference to tracked selection.
     *
     * Warning, references are invalidated on creation or deletion of other selection.
     */
    [[nodiscard]] auto selection(selection_id_t selection_id) -> Selection&;
    [[nodiscard]] auto selection(selection_id_t selection_id) const -> const Selection&;
    [[nodiscard]] auto create_selection() -> selection_id_t;
    [[nodiscard]] auto create_selection(Selection selection) -> selection_id_t;
    auto destroy_selection(selection_id_t selection_id) -> void;
    [[nodiscard]] auto selection_exists(selection_id_t selection_id) const -> bool;

    // visible selection
    auto set_visible_selection(Selection selection) -> void;
    auto clear_visible_selection() -> void;
    auto add_visible_selection_rect(SelectionFunction function, rect_fine_t rect) -> void;
    auto try_pop_last_visible_selection_rect() -> bool;
    auto try_update_last_visible_selection_rect(rect_fine_t rect) -> bool;
    auto apply_all_visible_selection_operations() -> void;
    [[nodiscard]] auto visible_selection() const -> const Selection&;
    [[nodiscard]] auto visible_selection_empty() const -> bool;

    // TODO don't give read access to cache
    [[nodiscard]] auto caches() const -> const LayoutIndex&;

   private:
    auto submit(const editable_circuit::InfoMessage& message) -> void;
    auto get_sender() -> editable_circuit::MessageSender&;
    auto get_state() -> editable_circuit::State;

    Layout layout_;
    LayoutIndex layout_index_;
    editable_circuit::MessageSender sender_;

    editable_circuit::SelectionStore selection_store_;
    VisibleSelection selection_builder_;
};

//
// Scoped Selection
//

class ScopedSelection {
   public:
    explicit ScopedSelection(EditableCircuit& editable_circuit);
    explicit ScopedSelection(EditableCircuit& editable_circuit, Selection selection);
    ~ScopedSelection();

    [[nodiscard]] auto selection_id() const -> selection_id_t;

   private:
    gsl::not_null<EditableCircuit*> editable_circuit_;
    selection_id_t selection_id_;
};

//
// Free functions
//

auto save_delete_all(EditableCircuit& editable_circuit, selection_id_t selection_id)
    -> void;

auto save_destroy_selection(EditableCircuit& editable_circuit,
                            selection_id_t selection_id) -> void;

auto visible_selection_select_all(EditableCircuit& editable_circuit) -> void;

auto visible_selection_delete_all(EditableCircuit& editable_circuit) -> void;

}  // namespace logicsim

#endif
