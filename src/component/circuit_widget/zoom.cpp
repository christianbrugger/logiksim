#include "component/circuit_widget/zoom.h"

#include "geometry/scene.h"
#include "qt/mouse_position.h"
#include "vocabulary/view_config.h"

namespace logicsim {

namespace circuit_widget {

namespace {

// factor the scale is multiplied or divided per zoom step
constexpr inline auto standard_zoom_factor = 1.1;

}  // namespace

auto zoom(ViewConfig view_config, double steps, QPointF center) -> ViewPoint {
    const auto factor = std::exp(steps * std::log(standard_zoom_factor));

    const auto old_grid_point = to_grid_fine(center, view_config);
    view_config.set_device_scale(view_config.device_scale() * factor);
    const auto new_grid_point = to_grid_fine(center, view_config);
    view_config.set_offset(view_config.offset() + new_grid_point - old_grid_point);

    return view_config.view_point();
}

auto zoom(const QWidget& widget, ViewConfig view_config, double steps) -> ViewPoint {
    const auto center = get_mouse_position_inside_widget(widget);
    return zoom(view_config, steps, center);
}

}  // namespace circuit_widget

}  // namespace logicsim
