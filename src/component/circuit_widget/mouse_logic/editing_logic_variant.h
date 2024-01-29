#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_VARIANT_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_VARIANT_H

#include "component/circuit_widget/mouse_logic/handle_resize.h"
#include "component/circuit_widget/mouse_logic/insert_logic_item.h"
#include "component/circuit_widget/mouse_logic/insert_wire.h"
#include "component/circuit_widget/mouse_logic/selection_area.h"
#include "component/circuit_widget/mouse_logic/selection_move.h"
#include "component/circuit_widget/mouse_logic/selection_single.h"

#include <variant>

namespace logicsim {

namespace circuit_widget {

using EditingMouseLogic =
    std::variant<InsertLogicItemLogic, InsertWireLogic, SelectionAreaLogic,
                 SelectionSingleLogic, SelectionMoveLogic, HandleResizeLogic>;

[[nodiscard]] auto is_insert_logic_item_logic(const EditingMouseLogic& logic) -> bool;
[[nodiscard]] auto is_insert_wire_logic(const EditingMouseLogic& logic) -> bool;
[[nodiscard]] auto is_selecting_logic(const EditingMouseLogic& logic) -> bool;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
