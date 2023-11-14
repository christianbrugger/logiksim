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

/**
 * @brief: Calculate zoom from given scroll wheel rotation at position.
 */
[[nodiscard]] auto wheel_zoom(QPointF position, QPoint angle_delta,
                              const ViewConfig& view_config) -> ViewPoint;

/**
 * @brief: Calculate surface offset from given pixel delta.
 */
[[nodiscard]] auto wheel_scroll_surface(QPoint pixel_delta, const ViewConfig& view_config)
    -> point_fine_t;
[[nodiscard]] auto wheel_scroll_surface_view_point(QPoint pixel_delta,
                                                   const ViewConfig& view_config)
    -> ViewPoint;

/**
 * @brief: Calculate horizontal offset from given scroll wheel rotation.
 */
[[nodiscard]] auto wheel_scroll_vertical(QPoint angle_delta,
                                         const ViewConfig& view_config) -> point_fine_t;
[[nodiscard]] auto wheel_scroll_vertical_view_point(QPoint angle_delta,
                                                    const ViewConfig& view_config)
    -> ViewPoint;

/**
 * @brief: Calculate vertical offset from given scroll wheel rotation.
 */
[[nodiscard]] auto wheel_scroll_horizontal(QPoint angle_delta,
                                           const ViewConfig& view_config) -> point_fine_t;
[[nodiscard]] auto wheel_scroll_horizontal_view_point(QPoint angle_delta,
                                                      const ViewConfig& view_config)
    -> ViewPoint;

/**
 * @brief: Calculate zoom and scroll offsets from mouse position,
 *          keyboard-modifiers and scroll-wheel rotations.
 */
[[nodiscard]] auto wheel_scroll_zoom(QPointF position, Qt::KeyboardModifiers modifiers,
                                     QPoint angle_delta,
                                     std::optional<QPoint> pixel_delta,
                                     const ViewConfig& view_config)
    -> std::optional<ViewPoint>;

/**
 * @brief: Calculate zoom and scroll offsets from wheel event.
 */
[[nodiscard]] auto wheel_scroll_zoom(const QWidget& widget, const QWheelEvent& event_,
                                     const ViewConfig& view_config)
    -> std::optional<ViewPoint>;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
