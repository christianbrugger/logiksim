#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_SETTING_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_SETTING_H

#include "gui/component/circuit_widget/mouse_logic/editing_logic_concept.h"

#include "core/setting_handle.h"
#include "core/vocabulary/point_fine.h"

#include <functional>
#include <optional>

namespace logicsim {

class EditableCircuit;

namespace circuit_widget {

using OpenSettingDialog = std::function<void(EditableCircuit&, setting_handle_t)>;

class HandleSettingLogic {
   public:
    explicit HandleSettingLogic(setting_handle_t setting_handle);

    auto mouse_press(EditableCircuit& editable_circuit, point_fine_t position) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, point_fine_t position,
                       const OpenSettingDialog& show_setting_dialog) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;

   private:
    setting_handle_t setting_handle_;

    std::optional<point_fine_t> first_position_ {};
};

static_assert(has_mouse_logic_finalize<HandleSettingLogic>);

}  // namespace circuit_widget

}  // namespace logicsim

#endif
