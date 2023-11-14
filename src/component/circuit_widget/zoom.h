#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_ZOOM_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_ZOOM_H

#include <QWidget>

namespace logicsim {

struct ViewPoint;
struct ViewConfig;

namespace circuit_widget {

[[nodiscard]] auto zoom(ViewConfig view_config, double steps, QPointF center)
    -> ViewPoint;

[[nodiscard]] auto zoom(const QWidget& widget, ViewConfig view_config, double steps)
    -> ViewPoint;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
