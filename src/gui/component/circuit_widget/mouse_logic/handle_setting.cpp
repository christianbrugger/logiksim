#include "component/circuit_widget/mouse_logic/handle_setting.h"

#include "editable_circuit.h"
#include "logging.h"

namespace logicsim {

namespace circuit_widget {

HandleSettingLogic::HandleSettingLogic(setting_handle_t setting_handle)
    : setting_handle_ {setting_handle} {}

auto HandleSettingLogic::mouse_press(EditableCircuit& editable_circuit [[maybe_unused]],
                                     point_fine_t position) -> void {
    first_position_ = position;
}

auto HandleSettingLogic::mouse_release(
    EditableCircuit& editable_circuit, point_fine_t position,
    const OpenSettingDialog& show_setting_dialog) -> void {
    if (first_position_ &&  //
        is_colliding(setting_handle_, first_position_.value()) &&
        is_colliding(setting_handle_, position)) {
        show_setting_dialog(editable_circuit, setting_handle_);
    }
}

auto HandleSettingLogic::finalize(EditableCircuit& editable_circuit
                                  [[maybe_unused]]) -> void {}

}  // namespace circuit_widget

}  // namespace logicsim
