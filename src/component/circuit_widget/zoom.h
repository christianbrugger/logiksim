#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_ZOOM_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_ZOOM_H

#include <QWidget>

namespace logicsim {

struct ViewPoint;
struct ViewConfig;

namespace circuit_widget {

/**
 * @brief: Calculate the zoomed view point at given position.
 */
[[nodiscard]] auto zoom(ViewConfig view_config, double steps, QPointF center)
    -> ViewPoint;

/**
 * @brief: Find a position within the widget and calculate the zoomed view point.
 * 
 * Note it uses the current mouse position or the center of the widget, if the
 * mouse is outside of the widget.
 */
[[nodiscard]] auto zoom(const QWidget& widget, ViewConfig view_config, double steps)
    -> ViewPoint;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
