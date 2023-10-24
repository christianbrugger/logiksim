#ifndef LOGICSIM_LOGIC_ITEM_SCHEMATIC_INFO_H
#define LOGICSIM_LOGIC_ITEM_SCHEMATIC_INFO_H

#include "vocabulary/delay.h"
#include "vocabulary/element_type.h"
#include "vocabulary/internal_connections.h"

namespace logicsim {

[[nodiscard]] auto element_output_delay(ElementType element_type) -> delay_t;

[[nodiscard]] auto element_internal_connections(ElementType element_type)
    -> internal_connections_t;

[[nodiscard]] auto has_internal_connections(ElementType element_type) -> bool;

}  // namespace logicsim

#endif
