#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_VARIANT_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_EDITING_LOGIC_VARIANT_H

#include "component/circuit_widget/mouse_logic/insert_logic_item.h"

#include <variant>

namespace logicsim {

namespace circuit_widget {

using EditingMouseLogic = std::variant<InsertLogicItemLogic>;

}

}  // namespace logicsim

#endif
