#ifndef LOGICSIM_LOGIC_ITEM_SCHEMATIC_INFO_H
#define LOGICSIM_LOGIC_ITEM_SCHEMATIC_INFO_H

#include "vocabulary/delay.h"
#include "vocabulary/element_type.h"

namespace logicsim {

[[nodiscard]] auto element_output_delay(ElementType element_type)->delay_t;


}

#endif
