#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_MANAGER_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_MANAGER_H

#include "component/circuit_widget/mouse_logic/editing_logic_variant.h"
#include "vocabulary/circuit_widget_state.h"

#include <QPointF>
#include <QRubberBand>
#include <QWidget>

#include <optional>

namespace logicsim {

struct ViewConfig;
class EditableCircuit;

namespace circuit_widget {

enum class ManagerResult {
    done,
    require_update,
};

/**
 * @brief: Manages the mouse interactions in the editing state
 *
 * Class-invariants:
 *   + mouse_logic_ is empty when not in editing-state
 *   + rubber_band is only shown in selection-state
 *
 * Note functions require valid editable_circuit in editing-mode and nullptr otherwise.
 */
class EditingLogicManager {
   public:
    EditingLogicManager(QWidget* parent);

    auto set_circuit_state(CircuitWidgetState new_state,
                           EditableCircuit* editable_circuit) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;

    auto finalize_editing(EditableCircuit* editable_circuit_) -> ManagerResult;

    [[nodiscard]] auto is_editing_active() const -> bool;

   public:
    [[nodiscard]] auto mouse_press(QPointF position, const ViewConfig& view_config,
                                   Qt::KeyboardModifiers modifiers, bool double_click,
                                   EditableCircuit* editable_circuit, QWidget& parent)
        -> ManagerResult;
    [[nodiscard]] auto mouse_move(QPointF position, const ViewConfig& view_config,
                                  EditableCircuit* editable_circuit) -> ManagerResult;
    [[nodiscard]] auto mouse_release(QPointF position, const ViewConfig& view_config,
                                     EditableCircuit* editable_circuit) -> ManagerResult;

   private:
    QRubberBand rubber_band_;
    CircuitWidgetState circuit_state_ {};

    std::optional<EditingMouseLogic> mouse_logic_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
