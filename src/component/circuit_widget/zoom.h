#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_ZOOM_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_ZOOM_H

#include <QWidget>

namespace logicsim {

struct point_device_fine_t;
struct ViewPoint;
struct ViewConfig;

namespace circuit_widget {

/**
 * @brief: Calculate the zoomed view point at given position.
 */
[[nodiscard]] auto zoom(ViewConfig view_config, double steps, point_device_fine_t center)
    -> ViewPoint;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
