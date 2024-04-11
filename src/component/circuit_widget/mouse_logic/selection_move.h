#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_MOVE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_MOVE_H

#include "component/circuit_widget/mouse_logic/editing_logic_interface.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/point_fine.h"

#include <optional>
#include <utility>
#include <vector>

namespace logicsim {

struct point_t;
class EditableCircuit;
class Selection;

namespace circuit_widget {

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
 */
class SelectionMoveLogic : public EditingLogicInterface {
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

    auto finalize(EditableCircuit& editable_circuit) -> void override;

   private:
    auto move_selection(EditableCircuit& editable_circuit, point_fine_t point) -> void;
    auto convert_selection_to(EditableCircuit& editable_circuit, InsertionMode new_mode)
        -> void;
    auto restore_original_positions(EditableCircuit& editable_circuit) -> void;

   private:
    bool delete_on_cancel_;
    State state_;
    InsertionMode insertion_mode_;

    std::optional<point_fine_t> last_position_ {};
    std::pair<int, int> total_offsets_ {};
    std::optional<std::vector<point_t>> cross_points_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
