#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_MOUSE_DRAG_LOGIC_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_MOUSE_DRAG_LOGIC_H

#include "vocabulary/view_config.h"

#include <QPointF>

#include <optional>

namespace logicsim {

namespace circuit_widget {

class MouseDragLogic {
   public:
    auto mouse_press(QPointF position) -> void;
    // returns new view-config offset
    auto mouse_move(QPointF position, const ViewConfig& config) -> point_fine_t;
    // returns new view-config offset
    auto mouse_release(QPointF position, const ViewConfig& config) -> point_fine_t;

   private:
    std::optional<QPointF> last_position_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
