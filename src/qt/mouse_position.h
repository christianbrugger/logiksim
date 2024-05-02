#ifndef LOGICSIM_QT_MOUSE_POSITION_H
#define LOGICSIM_QT_MOUSE_POSITION_H

#include <string_view>

class QWidget;
class QSinglePointEvent;
class QWheelEvent;
class QMouseEvent;
class QPointF;

namespace logicsim {

struct MousePositionInfo;

[[nodiscard]] auto get_mouse_position(const QWidget* widget, const QWheelEvent* event_)
    -> QPointF;
[[nodiscard]] auto get_mouse_position(const QWidget& widget, const QWheelEvent& event_)
    -> QPointF;

[[nodiscard]] auto get_mouse_position(const QWidget* widget, const QMouseEvent* event_)
    -> QPointF;
[[nodiscard]] auto get_mouse_position(const QWidget& widget, const QMouseEvent& event_)
    -> QPointF;

[[nodiscard]] auto get_mouse_position(const QWidget& widget) -> QPointF;

[[nodiscard]] auto get_mouse_position_inside_widget(const QWidget& widget) -> QPointF;

[[nodiscard]] auto create_mouse_position_info(std::string_view source, QPointF position,
                                              QSinglePointEvent* event_ = nullptr)
    -> MousePositionInfo;

}  // namespace logicsim

#endif
