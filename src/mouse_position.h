#ifndef LOGICSIM_MOUSE_POSITION_H
#define LOGICSIM_MOUSE_POSITION_H

#include <QSinglePointEvent>
#include <QWidget>

namespace logicsim {

[[nodiscard]] auto get_mouse_position(const QWidget* widget,
                                      const QSinglePointEvent* event_) -> QPointF;
[[nodiscard]] auto get_mouse_position(const QWidget& widget,
                                      const QSinglePointEvent& event_) -> QPointF;

[[nodiscard]] auto get_mouse_position(const QWidget& widget) -> QPointF;

[[nodiscard]] auto get_mouse_position_inside_widget(const QWidget& widget) -> QPointF;

}  // namespace logicsim

#endif
