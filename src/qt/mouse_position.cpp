#include "qt/mouse_position.h"

#include "format/qt_type.h"
#include "logging.h"
#include "qt/point_conversion.h"
#include "vocabulary/mouse_postion_info.h"

#include <gsl/gsl>

#include <QMouseEvent>
#include <QPointF>
#include <QSinglePointEvent>
#include <QWheelEvent>
#include <QWidget>

namespace logicsim {

[[nodiscard]] auto get_mouse_position(const QWidget* widget, const QWheelEvent* event_)
    -> QPointF {
    Expects(widget);
    Expects(event_);

    return get_mouse_position(*widget, *event_);
}

[[nodiscard]] auto get_mouse_position(const QWidget* widget, const QMouseEvent* event_)
    -> QPointF {
    Expects(widget);
    Expects(event_);

    return get_mouse_position(*widget, *event_);
}

[[nodiscard]] auto get_mouse_position(const QWidget& widget, const QWheelEvent& event_)
    -> QPointF {
    // print();
    // print("position      ", event_.position());
    // print("scenePosition ", event_.scenePosition());
    // print("globalPosition", event_.globalPosition());

    // for WheelEvents globalPosition is the only function of event_ that returns
    // non-rounded positions for display scaling.
    return widget.mapFromGlobal(event_.globalPosition());
}

[[nodiscard]] auto get_mouse_position(const QWidget& widget, const QMouseEvent& event_)
    -> QPointF {
    // for MouseEvents scenePosition is the only function of event_ that returns
    // non-rounded positions for display scaling.
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

auto create_mouse_position_info(std::string_view source, QPointF position,
                                QSinglePointEvent* event_) -> MousePositionInfo {
    auto info = MousePositionInfo {
        .position = to(position),
        .labels = {std::string(source),
                   mouse_position_label("device", "point_device_fine_t", to(position))},
    };

    if (event_ != nullptr) {
        info.labels.emplace_back(
            mouse_position_label("event->position", "QPointF", to(event_->position())));
        info.labels.emplace_back(mouse_position_label("event->scenePosition", "QPointF",
                                                      to(event_->scenePosition())));
        info.labels.emplace_back(mouse_position_label("event->globalPosition", "QPointF",
                                                      to(event_->globalPosition())));
    }

    return info;
}

}  // namespace logicsim
