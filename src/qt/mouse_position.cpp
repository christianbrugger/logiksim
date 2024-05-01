#include "qt/mouse_position.h"

#include "format/qt_type.h"
#include "logging.h"

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
    print("position", event_.position());
    print("scene", event_.scenePosition());
    print("global", event_.globalPosition());
    print();

    if (event_.type() == QEvent::Wheel) {
        // Wheel events scenePosition returns the same as position.
        // I don't know why, it seems to be a bug.
        return event_.scenePosition();
    }

    // scenePosition is the only function of event_ that returns non-rounded
    // positions with display-scaling > 1.
    return widget.mapFrom(widget.topLevelWidget(), event_.scenePosition());
}

auto get_mouse_position(const QWidget& widget) -> QPointF {
    return widget.mapFromGlobal(QPointF {QCursor::pos()});
}

auto get_mouse_position_inside_widget(const QWidget& widget) -> QPointF {
    const auto mouse_position = get_mouse_position(widget);

    if (QRectF(widget.rect()).contains(mouse_position)) {
        return mouse_position;
    }

    return QPointF {widget.width() / 2., widget.height() / 2.};
}

}  // namespace logicsim
