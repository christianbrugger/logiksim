#include "core/component/circuit_ui_model/mouse_logic/editing_logic_variant.h"

#include "core/component/circuit_ui_model/mouse_logic/editing_logic_concept.h"

namespace logicsim {

namespace circuit_ui_model {

auto is_insert_logicitem_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<InsertLogicItemLogic>(logic);
}

auto is_insert_wire_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<InsertWireLogic>(logic);
}

auto is_insert_decoration_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<InsertDecorationLogic>(logic);
}

auto is_selecting_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<SelectionAreaLogic>(logic) ||
           std::holds_alternative<SelectionSingleLogic>(logic) ||
           std::holds_alternative<SelectionMoveLogic>(logic) ||
           std::holds_alternative<HandleResizeLogic>(logic) ||
           std::holds_alternative<HandleSettingLogic>(logic);
}

}  // namespace circuit_ui_model

}  // namespace logicsim
