#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_MOUSE_DRAG_LOGIC_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_MOUSE_DRAG_LOGIC_H

#include "vocabulary/point_device_fine.h"
#include "vocabulary/view_config.h"

#include <optional>

namespace logicsim {

namespace circuit_widget {

/**
 * @brief: Calculates updated view-config offsets for mouse drags.
 *
 * Note the returned offsets need to be applied after each call for this to work.
 */
class MouseDragLogic {
   public:
    auto mouse_press(point_device_fine_t position) -> void;

    /**
     * @brief: Updates the drag position and returns the updated view-config offset.
     */
    auto mouse_move(point_device_fine_t position, const ViewConfig& config)
        -> point_fine_t;

    /**
     * @brief: Finalizes the drag position and returns the updated view-config offset.
     */
    auto mouse_release(point_device_fine_t position, const ViewConfig& config)
        -> point_fine_t;

   private:
    std::optional<point_device_fine_t> last_position_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
