#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_SETTING_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_SETTING_H

#include "component/circuit_widget/mouse_logic/editing_logic_interface.h"

namespace logicsim {

struct point_fine_t;
class EditableCircuit;

namespace circuit_widget {

class HandleSettingLogic : public EditingLogicInterface {
   public:
    auto mouse_press(EditableCircuit& editable_circuit, point_fine_t point) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, point_fine_t point) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void override;
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
