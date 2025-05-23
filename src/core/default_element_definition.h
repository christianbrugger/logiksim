#ifndef LOGICSIM_DEFAULT_ELEMENT_DEFINITION_H
#define LOGICSIM_DEFAULT_ELEMENT_DEFINITION_H

#include "core/vocabulary/decoration_type.h"
#include "core/vocabulary/default_mouse_action.h"
#include "core/vocabulary/logicitem_type.h"

namespace logicsim {

struct LogicItemDefinition;
struct DecorationDefinition;

[[nodiscard]] auto default_element_definition(LogicItemType logicitem_type)
    -> LogicItemDefinition;

[[nodiscard]] auto to_logicitem_definition(DefaultMouseAction mouse_action)
    -> LogicItemDefinition;

[[nodiscard]] auto default_decoration_definition(DecorationType decoration_type)
    -> DecorationDefinition;

[[nodiscard]] auto to_decoration_definition(DefaultMouseAction mouse_action)
    -> DecorationDefinition;

}  // namespace logicsim

#endif
