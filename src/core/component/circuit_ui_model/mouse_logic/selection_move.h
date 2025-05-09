#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_SELECTION_MOVE_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_SELECTION_MOVE_H

#include "core/component/circuit_ui_model/mouse_logic/editing_logic_concept.h"
#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/move_delta.h"
#include "core/vocabulary/point_fine.h"

#include <optional>
#include <utility>
#include <vector>

namespace logicsim {

struct point_t;
class EditableCircuit;
class Selection;

namespace circuit_ui_model {

namespace selection_move_logic {

enum class State {
    waiting_for_first_click,
    move_selection,
    waiting_for_confirmation,
    finished,
    finished_confirmed,
};

struct Args {
    /**
     * @brief: Needs to be set if visible selection contains any colliding / valid items.
     */
    bool has_colliding {false};
    /**
     * @brief: If set deletes
     */
    bool delete_on_cancel {false};
    /**
     * @brief: When has_colliding is set to true this method requires a list of
     *         true cross-points, so they can be restored on insert / un-insert.
     *
     * Needs to be set, potentially empty for has_colliding. And nullopt otherwise.
     */
    std::optional<std::vector<point_t>> cross_points {};
};

}  // namespace selection_move_logic

/**
 * @ brief: Logic to handle selection moving via mouse clicks.
 *
 *
 * Pre-condition:
 *   + history enable state is not changed during this logic is active
 */
class SelectionMoveLogic {
   public:
    using State = selection_move_logic::State;
    using Args = selection_move_logic::Args;

   public:
    explicit SelectionMoveLogic(const EditableCircuit& editable_circuit, Args args = {});

    auto mouse_press(EditableCircuit& editable_circuit, point_fine_t point,
                     bool double_click) -> void;
    auto mouse_move(EditableCircuit& editable_circuit, point_fine_t point) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, point_fine_t point) -> void;

    [[nodiscard]] auto is_finished() const -> bool;
    auto confirm() -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;

   private:
    auto move_selection(EditableCircuit& editable_circuit, point_fine_t point) -> void;
    auto convert_selection_to(EditableCircuit& editable_circuit,
                              InsertionMode new_mode) -> void;
    auto restore_original_positions(EditableCircuit& editable_circuit) -> void;

   private:
    bool delete_on_cancel_;
    State state_;
    InsertionMode insertion_mode_;
    bool initial_history_enabled_;
    bool expected_history_enabled_;

    std::optional<point_fine_t> last_position_ {};
    move_delta_t total_offsets_ {};
    move_delta_t history_offsets_ {};
    std::optional<std::vector<point_t>> cross_points_ {};
};

static_assert(has_mouse_logic_finalize<SelectionMoveLogic>);

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
