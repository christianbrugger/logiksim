#ifndef LOGICSIM_CORE_LOGIC_ITEM_RENDER_RENDER_LOGICITEM_OVERLAY_H
#define LOGICSIM_CORE_LOGIC_ITEM_RENDER_RENDER_LOGICITEM_OVERLAY_H

#include "core/vocabulary/logicitem_type.h"

namespace logicsim {

struct grid_fine_t;

auto element_shadow_rounding(LogicItemType type) -> grid_fine_t;

}  // namespace logicsim

#endif
