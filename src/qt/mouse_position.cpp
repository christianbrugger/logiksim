#include "qt/mouse_position.h"

#include "format/qt_type.h"
#include "logging.h"
#include "qt/mouse_position_p.h"
#include "qt/point_conversion.h"
#include "vocabulary/mouse_postion_info.h"

#include <gsl/gsl>

#include <QMouseEvent>
#include <QPointF>
#include <QSinglePointEvent>
#include <QWheelEvent>
#include <QWidget>

namespace logicsim {

[[nodiscard]] auto get_mouse_position(const QWidget* widget, const QMouseEvent* event_)
    -> QPointF {
    Expects(widget);
    Expects(event_);

    return get_mouse_position(*widget, *event_);
}

[[nodiscard]] auto get_mouse_position(const QWidget* widget, const QWheelEvent* event_)
    -> QPointF {
    Expects(widget);
    Expects(event_);

    return get_mouse_position(*widget, *event_);
}

namespace {

auto map_from_top_level_high_dpi(const QWidget& widget, QPointF scene_position)
    -> QPointF {
    // Simply calling widget.mapFrom(widget.topLevelWidget(), ..) unfortunately doesn't
    // work for 150% display scaling as `mapFrom` works on device independent geometry.

    // All render code works on device coordinates. So we also need to round the upper
    // left corner (0, 0) of our widget to device coordinates. That is the true position
    // of the corner pixel. And thats what we use for transformation.

    const auto tlw = widget.topLevelWidget();
    Expects(tlw != nullptr);

    const auto offset = widget.mapTo(tlw, QPointF {0, 0});
    const auto ratio = widget.devicePixelRatioF();
    const auto offset_rounded = QPointF {(offset * ratio).toPoint()} / ratio;

    return scene_position - offset_rounded;
}

auto map_from_global_high_dpi(const QWidget& widget, QPointF global_position) -> QPointF {
    // `widget.mapFromGlobal` uses rounded geometry to map from the top level widget
    // to the widget. We therefore need to use our fixed method for that.
    const auto tlw = widget.topLevelWidget();
    Expects(tlw != nullptr);

    const auto scene_position = tlw->mapFromGlobal(global_position);
    return map_from_top_level_high_dpi(widget, scene_position);
}

}  // namespace

[[nodiscard]] auto get_mouse_position(const QWidget& widget, const QMouseEvent& event_)
    -> QPointF {
    // for MouseEvents scenePosition is the only function of event_ that returns
    // non-rounded positions for display scaling.
    return map_from_top_level_high_dpi(widget, event_.scenePosition());
}

[[nodiscard]] auto get_mouse_position(const QWidget& widget, const QWheelEvent& event_)
    -> QPointF {
    // for WheelEvents globalPosition is the only function of event_ that returns
    // non-rounded positions for display scaling.
    return map_from_global_high_dpi(widget, event_.globalPosition());
}

/**
 * @brief: Get current cursor position in relation to the widget.
 *
 * Uses private cursor_position implementation to get accurate global mouse position.
 *
 * Portable alternative, with integer rounding:
 *
 *      return widget.mapFromGlobal(QPointF {QCursor::pos()});
 */
auto get_mouse_position(const QWidget& widget) -> QPointF {
    return map_from_global_high_dpi(widget, cursor_position_high_dpi());
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
