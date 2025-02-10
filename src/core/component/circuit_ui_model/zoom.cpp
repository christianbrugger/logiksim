#include "core/component/circuit_ui_model/zoom.h"

#include "core/geometry/scene.h"
#include "core/vocabulary/point_device_fine.h"
#include "core/vocabulary/view_config.h"

namespace logicsim {

namespace circuit_ui_model {

namespace {

// factor the scale is multiplied or divided per zoom step
constexpr inline auto standard_zoom_factor = 1.1;

}  // namespace

auto zoom(ViewConfig view_config, double steps, point_device_fine_t center) -> ViewPoint {
    const auto factor = std::exp(steps * std::log(standard_zoom_factor));

    const auto old_grid_point = to_grid_fine(center, view_config);
    view_config.set_device_scale(view_config.device_scale() * factor);
    const auto new_grid_point = to_grid_fine(center, view_config);
    view_config.set_offset(view_config.offset() + new_grid_point - old_grid_point);

    return view_config.view_point();
}

}  // namespace circuit_ui_model

}  // namespace logicsim
