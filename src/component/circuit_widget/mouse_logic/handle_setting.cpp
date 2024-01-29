#include "component/circuit_widget/mouse_logic/handle_setting.h"

#include "editable_circuit.h"
#include "logging.h"

namespace logicsim {

namespace circuit_widget {

HandleSettingLogic::HandleSettingLogic(setting_handle_t setting_handle)
    : setting_handle_ {setting_handle} {}

auto HandleSettingLogic::mouse_press(EditableCircuit& editable_circuit,
                                     point_fine_t position) -> void {
    first_position_ = position;
}

auto HandleSettingLogic::mouse_release(EditableCircuit& editable_circuit,
                                       point_fine_t position) -> void {
    if (first_position_ &&  //
        is_colliding(setting_handle_, first_position_.value()) &&
        is_colliding(setting_handle_, position)) {
        // widget_registry_.show_setting_dialog(setting_handle_, editable_circuit);
        print("setting handle at", setting_handle_.position);
    }
}

auto HandleSettingLogic::finalize(EditableCircuit& editable_circuit) -> void {}

}  // namespace circuit_widget

}  // namespace logicsim
