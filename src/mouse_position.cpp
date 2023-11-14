#include "mouse_position.h"

#include <gsl/gsl>

namespace logicsim {

auto get_mouse_position(const QWidget* widget, const QSinglePointEvent* event_)
    -> QPointF {
    Expects(widget);
    Expects(event_);

    return get_mouse_position(*widget, *event_);
}

auto get_mouse_position(const QWidget& widget, const QSinglePointEvent& event_)
    -> QPointF {
    return widget.mapFromGlobal(event_.globalPosition());
}

auto get_mouse_position(const QWidget& widget) -> QPointF {
    return widget.mapFromGlobal(QPointF {QCursor::pos()});
}

auto get_mouse_position_inside_widget(const QWidget& widget) -> QPointF {
    const auto mouse_position = get_mouse_position(widget);

    if (QRectF(widget.rect()).contains(mouse_position)) {
        return mouse_position;
    }

    return QPointF(widget.width() / 2., widget.height() / 2.);
}

}  // namespace logicsim
