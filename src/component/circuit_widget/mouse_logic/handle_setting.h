#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_SETTING_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_SETTING_H

#include "component/circuit_widget/mouse_logic/editing_logic_interface.h"
#include "setting_handle.h"
#include "vocabulary/point_fine.h"

#include <optional>

namespace logicsim {

class EditableCircuit;

namespace circuit_widget {

class HandleSettingLogic : public EditingLogicInterface {
   public:
    explicit HandleSettingLogic(setting_handle_t setting_handle);

    auto mouse_press(EditableCircuit& editable_circuit, point_fine_t position) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, point_fine_t position) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void override;

   private:
    setting_handle_t setting_handle_;

    std::optional<point_fine_t> first_position_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
