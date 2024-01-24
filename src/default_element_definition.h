#ifndef LOGICSIM_DEFAULT_ELEMENT_DEFINITION_H
#define LOGICSIM_DEFAULT_ELEMENT_DEFINITION_H

#include "vocabulary/default_mouse_action.h"
#include "vocabulary/logicitem_type.h"

namespace logicsim {

struct LogicItemDefinition;

auto default_element_definition(LogicItemType logicitem_type) -> LogicItemDefinition;

auto to_logic_item_definition(DefaultMouseAction mouse_action) -> LogicItemDefinition;

}  // namespace logicsim

#endif
