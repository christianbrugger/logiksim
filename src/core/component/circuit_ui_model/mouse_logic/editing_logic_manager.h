#ifndef LOGICSIM_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_EDITING_LOGIC_MANAGER_H
#define LOGICSIM_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_EDITING_LOGIC_MANAGER_H

#include "core/component/circuit_ui_model/mouse_logic/editing_logic_variant.h"
#include "core/vocabulary/circuit_widget_state.h"

#include <optional>

namespace logicsim {

struct point_device_fine_t;
class KeyboardModifiers;
struct ViewConfig;
class EditableCircuit;

namespace circuit_ui_model {

struct mouse_logic_result_t;

/**
 * @brief: Manages the mouse interactions in the editing state
 *
 * Class-invariants:
 *   + mouse_logic_ is empty when not in editing-state
 *   + rubber_band is only shown if SelectionAreaLogic is active
 *
 * Note functions require valid editable_circuit in editing-mode and nullptr otherwise.
 */
class EditingLogicManager {
   public:
    auto set_circuit_state(CircuitWidgetState new_state,
                           EditableCircuit* editable_circuit) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;

    auto finalize_editing(EditableCircuit* editable_circuit_) -> mouse_logic_result_t;
    auto confirm_editing(EditableCircuit* editable_circuit_) -> mouse_logic_result_t;

    [[nodiscard]] auto is_editing_active() const -> bool;
    [[nodiscard]] auto is_area_selection_active() const -> bool;

    /**
     * @brief: Setup a move for a colliding visible selection.
     *
     * Pre-condition:
     *   + Visible selection is in InsertionMode::collisions
     *   + circuit_state() == selection mode
     *   + is_editing_active() is false
     */
    auto setup_colliding_move(const EditableCircuit& editable_circuit_,
                              std::vector<point_t> cross_points) -> void;

   public:
    [[nodiscard]] auto mouse_press(
        point_device_fine_t position, const ViewConfig& view_config,
        KeyboardModifiers modifiers, bool double_click,
        EditableCircuit* editable_circuit) -> mouse_logic_result_t;

    [[nodiscard]] auto mouse_move(
        point_device_fine_t position, const ViewConfig& view_config,
        EditableCircuit* editable_circuit) -> mouse_logic_result_t;

    [[nodiscard]] auto mouse_release(
        point_device_fine_t position, const ViewConfig& view_config,
        EditableCircuit* editable_circuit,
        const OpenSettingDialog& show_setting_dialog) -> mouse_logic_result_t;

   private:
    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    CircuitWidgetState circuit_state_ {};

    std::optional<EditingMouseLogic> mouse_logic_ {};
};

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
