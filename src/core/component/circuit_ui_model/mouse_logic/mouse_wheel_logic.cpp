#include "core/component/circuit_ui_model/mouse_logic/mouse_wheel_logic.h"

#include "core/component/circuit_ui_model/zoom.h"
#include "core/geometry/scene.h"
#include "core/vocabulary/view_config.h"

namespace logicsim {

namespace circuit_ui_model {

namespace {

// device pixels to scroll for one scroll
constexpr inline auto standard_scroll_pixel = 45;

[[nodiscard]] auto get_standard_scroll(const ViewConfig& view_config) -> grid_fine_t {
    return grid_fine_t {standard_scroll_pixel / view_config.device_scale()};
}

}  // namespace

auto wheel_zoom(const point_device_fine_t& position, angle_delta_t angle_delta,
                const ViewConfig& view_config) -> ViewPoint {
    const auto steps = double {angle_delta.vertical_notches};
    return zoom(view_config, steps, position);
}

auto wheel_scroll_vertical(angle_delta_t angle_delta,
                           const ViewConfig& view_config) -> point_fine_t {
    const auto standard_scroll_grid = get_standard_scroll(view_config);
    const auto moved = point_fine_t {
        standard_scroll_grid * angle_delta.horizontal_notches,
        standard_scroll_grid * angle_delta.vertical_notches,
    };
    return view_config.offset() + moved;
}

auto wheel_scroll_vertical_view_point(angle_delta_t angle_delta,
                                      const ViewConfig& view_config) -> ViewPoint {
    return ViewPoint {
        .offset = wheel_scroll_vertical(angle_delta, view_config),
        .device_scale = view_config.device_scale(),
    };
}

auto wheel_scroll_horizontal(angle_delta_t angle_delta,
                             const ViewConfig& view_config) -> point_fine_t {
    const auto standard_scroll_grid = get_standard_scroll(view_config);
    auto moved = point_fine_t {
        standard_scroll_grid * angle_delta.vertical_notches,
        standard_scroll_grid * angle_delta.horizontal_notches,
    };
    return view_config.offset() + moved;
}

auto wheel_scroll_horizontal_view_point(angle_delta_t angle_delta,
                                        const ViewConfig& view_config) -> ViewPoint {
    return ViewPoint {
        .offset = wheel_scroll_horizontal(angle_delta, view_config),
        .device_scale = view_config.device_scale(),
    };
}

auto wheel_scroll_zoom(const MouseWheelEvent& event,
                       const ViewConfig& view_config) -> std::optional<ViewPoint> {
    if (event.modifiers == KeyboardModifier::Control) {
        return wheel_zoom(event.position, event.angle_delta, view_config);
    }

    if (event.modifiers == KeyboardModifiers {}) {
        return wheel_scroll_vertical_view_point(event.angle_delta, view_config);
    }

    if (event.modifiers == KeyboardModifier::Shift) {
        return wheel_scroll_horizontal_view_point(event.angle_delta, view_config);
    }

    return std::nullopt;
}

}  // namespace circuit_ui_model

}  // namespace logicsim
