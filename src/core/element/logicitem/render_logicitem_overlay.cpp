#include "element/logicitem/render_logicitem_overlay.h"

#include "layout_info.h"
#include "vocabulary/grid_fine.h"

namespace logicsim {

auto element_shadow_rounding(LogicItemType type) -> grid_fine_t {
    // TODO: why line_selection_padding and not logicitem overdraw?
    // TODO: why are those methods defined in layout_info and not here? Where needed?
    return type == LogicItemType::button ? grid_fine_t {0.} : line_selection_padding();
}

}  // namespace logicsim
