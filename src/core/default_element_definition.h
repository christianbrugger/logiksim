#ifndef LOGICSIM_DEFAULT_ELEMENT_DEFINITION_H
#define LOGICSIM_DEFAULT_ELEMENT_DEFINITION_H

#include "vocabulary/decoration_type.h"
#include "vocabulary/default_mouse_action.h"
#include "vocabulary/logicitem_type.h"

namespace logicsim {

struct LogicItemDefinition;
struct DecorationDefinition;

[[nodiscard]] auto default_element_definition(LogicItemType logicitem_type)
    -> LogicItemDefinition;

[[nodiscard]] auto to_logic_item_definition(DefaultMouseAction mouse_action)
    -> LogicItemDefinition;

[[nodiscard]] auto default_decoration_definition(DecorationType decoration_type)
    -> DecorationDefinition;

[[nodiscard]] auto to_decoration_definition(DefaultMouseAction mouse_action)
    -> DecorationDefinition;

}  // namespace logicsim

#endif
