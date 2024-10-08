#include "element/logicitem/render_logicitem_layer.h"

namespace logicsim {

auto draw_logicitem_above(LogicItemType type) -> bool {
    using enum LogicItemType;
    return type == button || type == led;
}

}  // namespace logicsim
