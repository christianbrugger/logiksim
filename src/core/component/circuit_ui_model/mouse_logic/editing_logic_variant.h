#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_EDITING_LOGIC_VARIANT_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_EDITING_LOGIC_VARIANT_H

#include "core/component/circuit_ui_model/mouse_logic/handle_resize.h"
#include "core/component/circuit_ui_model/mouse_logic/handle_setting.h"
#include "core/component/circuit_ui_model/mouse_logic/insert_decoration.h"
#include "core/component/circuit_ui_model/mouse_logic/insert_logicitem.h"
#include "core/component/circuit_ui_model/mouse_logic/insert_wire.h"
#include "core/component/circuit_ui_model/mouse_logic/selection_area.h"
#include "core/component/circuit_ui_model/mouse_logic/selection_move.h"
#include "core/component/circuit_ui_model/mouse_logic/selection_single.h"

#include <variant>

namespace logicsim {

namespace circuit_ui_model {

using EditingMouseLogic =
    std::variant<InsertLogicItemLogic, InsertWireLogic, InsertDecorationLogic,
                 SelectionAreaLogic, SelectionSingleLogic, SelectionMoveLogic,
                 HandleResizeLogic, HandleSettingLogic>;

[[nodiscard]] auto is_insert_logicitem_logic(const EditingMouseLogic& logic) -> bool;
[[nodiscard]] auto is_insert_wire_logic(const EditingMouseLogic& logic) -> bool;
[[nodiscard]] auto is_insert_decoration_logic(const EditingMouseLogic& logic) -> bool;
[[nodiscard]] auto is_selecting_logic(const EditingMouseLogic& logic) -> bool;

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
