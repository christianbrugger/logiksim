#include "logic_item/render/render_logicitem_layer.h"

namespace logicsim {

auto draw_logic_item_above(LogicItemType type) -> bool {
    using enum LogicItemType;
    return type == button || type == led;
}

}  // namespace logicsim
