#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_MOUSE_WHEEL_LOGIC_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_MOUSE_WHEEL_LOGIC_H

#include <QWheelEvent>
#include <QWidget>
#include <Qt>

#include <optional>

namespace logicsim {

struct ViewConfig;
struct ViewPoint;
struct point_fine_t;

namespace circuit_widget {

[[nodiscard]] auto wheel_zoom(QPointF position, QPoint angle_delta,
                              const ViewConfig& view_config) -> ViewPoint;

[[nodiscard]] auto wheel_scroll_surface(QPoint pixel_delta, const ViewConfig& view_config)
    -> point_fine_t;

[[nodiscard]] auto wheel_scroll_vertical(QPoint angle_delta,
                                         const ViewConfig& view_config) -> point_fine_t;

[[nodiscard]] auto wheel_scroll_horizontal(QPoint angle_delta,
                                           const ViewConfig& view_config) -> point_fine_t;

[[nodiscard]] auto wheel_scroll_zoom(QPointF position, Qt::KeyboardModifiers modifiers,
                                  QPoint angle_delta, std::optional<QPoint> pixel_delta,
                                  const ViewConfig& view_config)
    -> std::optional<ViewPoint>;

[[nodiscard]] auto wheel_scroll_zoom(const QWidget& widget, const QWheelEvent& event_,
                                  const ViewConfig& view_config)
    -> std::optional<ViewPoint>;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
