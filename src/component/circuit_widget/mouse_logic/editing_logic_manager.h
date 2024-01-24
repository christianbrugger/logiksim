#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_MANAGER_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_MANAGER_H

#include "component/circuit_widget/mouse_logic/editing_logic_variant.h"
#include "vocabulary/circuit_widget_state.h"

#include <QPointF>
#include <Qt>

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
 *   + editing_mouse_logic_ is empty when not in editing state
 */
class EditingLogicManager {
   public:
    auto set_circuit_state(CircuitWidgetState new_state) -> void;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;

    auto mouse_press(QPointF position, const ViewConfig& view_config,
                     Qt::MouseButton button, EditableCircuit& editable_circuit)
        -> ManagerResult;
    auto mouse_move(QPointF position, const ViewConfig& view_config,
                    EditableCircuit& editable_circuit) -> ManagerResult;
    auto mouse_release(QPointF position, const ViewConfig& view_config,
                       EditableCircuit& editable_circuit) -> ManagerResult;

   private:
    CircuitWidgetState circuit_state_ {};

    std::optional<EditingMouseLogic> mouse_logic_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
