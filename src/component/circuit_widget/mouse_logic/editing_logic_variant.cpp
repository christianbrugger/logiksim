#include "component/circuit_widget/mouse_logic/editing_logic_variant.h"

namespace logicsim {

namespace circuit_widget {

auto is_insert_logic_item_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<InsertLogicItemLogic>(logic);
}

auto is_insert_wire_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<InsertWireLogic>(logic);
}

auto is_selecting_logic(const EditingMouseLogic& logic) -> bool {
    return std::holds_alternative<SelectionAreaLogic>(logic) ||
           std::holds_alternative<SelectionSingleLogic>(logic);
}

}  // namespace circuit_widget

}  // namespace logicsim
