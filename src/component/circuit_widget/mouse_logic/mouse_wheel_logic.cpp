#include "component/circuit_widget/mouse_logic/mouse_wheel_logic.h"

#include "component/circuit_widget/zoom.h"
#include "geometry/scene.h"
#include "mouse_position.h"
#include "vocabulary/view_config.h"

#include <QCursor>
#include <QWidget>

namespace logicsim {

namespace circuit_widget {

namespace {

// device pixels to scroll for one scroll
constexpr inline auto standard_scroll_pixel = 45;

// degree delta for one scroll
constexpr inline auto standard_delta = 120.0;

[[nodiscard]] auto get_standard_scroll(const ViewConfig& view_config) -> grid_fine_t {
    return grid_fine_t {standard_scroll_pixel / view_config.device_scale()};
}

}  // namespace

auto wheel_zoom(QPointF position, QPoint angle_delta, const ViewConfig& view_config)
    -> ViewPoint {
    const auto steps = angle_delta.y() / standard_delta;
    return zoom(view_config, steps, position);
}

auto wheel_scroll_surface(QPoint pixel_delta, const ViewConfig& view_config)
    -> point_fine_t {
    // TODO test this
    const auto scale = view_config.device_scale();
    const auto moved = point_fine_t {
        pixel_delta.x() / scale,
        pixel_delta.y() / scale,
    };
    return view_config.offset() + moved;
}

auto wheel_scroll_vertical(QPoint angle_delta, const ViewConfig& view_config)
    -> point_fine_t {
    const auto standard_scroll_grid = get_standard_scroll(view_config);
    const auto moved = point_fine_t {
        standard_scroll_grid * angle_delta.x() / standard_delta,
        standard_scroll_grid * angle_delta.y() / standard_delta,
    };
    return view_config.offset() + moved;
}

auto wheel_scroll_horizontal(QPoint angle_delta, const ViewConfig& view_config)
    -> point_fine_t {
    const auto standard_scroll_grid = get_standard_scroll(view_config);
    auto moved = point_fine_t {
        standard_scroll_grid * angle_delta.y() / standard_delta,
        standard_scroll_grid * angle_delta.x() / standard_delta,
    };
    return view_config.offset() + moved;
}

auto wheel_scroll_zoom(QPointF position, Qt::KeyboardModifiers modifiers,
                       QPoint angle_delta, std::optional<QPoint> pixel_delta,
                       const ViewConfig& view_config) -> std::optional<ViewPoint> {
    if (modifiers == Qt::ControlModifier) {
        return wheel_zoom(position, angle_delta, view_config);
    }

    else if (modifiers == Qt::NoModifier) {
        if (pixel_delta) {
            return ViewPoint {
                .offset = wheel_scroll_surface(*pixel_delta, view_config),
                .device_scale = view_config.device_scale(),
            };
        }
        return ViewPoint {
            .offset = wheel_scroll_vertical(angle_delta, view_config),
            .device_scale = view_config.device_scale(),
        };
    }

    else if (modifiers == Qt::ShiftModifier) {
        return ViewPoint {
            .offset = wheel_scroll_horizontal(angle_delta, view_config),
            .device_scale = view_config.device_scale(),
        };
    }

    return std::nullopt;
}

auto wheel_scroll_zoom(const QWidget& widget, const QWheelEvent& event_,
                       const ViewConfig& view_config) -> std::optional<ViewPoint> {
    const auto position = get_mouse_position(widget, event_);
    const auto pixel_delta =
        event_.hasPixelDelta() ? std::make_optional(event_.pixelDelta()) : std::nullopt;

    return circuit_widget::wheel_scroll_zoom(
        position, event_.modifiers(), event_.angleDelta(), pixel_delta, view_config);
}

}  // namespace circuit_widget

}  // namespace logicsim
