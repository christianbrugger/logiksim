#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_MOUSE_WHEEL_LOGIC_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_MOUSE_WHEEL_LOGIC_H

#include "core/vocabulary/mouse_event.h"

#include <optional>

namespace logicsim {

struct ViewConfig;
struct ViewPoint;
struct point_fine_t;

namespace circuit_ui_model {

/**
 * @brief: Calculate zoom from given scroll wheel rotation at position.
 */
[[nodiscard]] auto wheel_zoom(const point_device_fine_t& position,
                              angle_delta_t angle_delta,
                              const ViewConfig& view_config) -> ViewPoint;

/**
 * @brief: Calculate horizontal offset from given scroll wheel rotation.
 */
[[nodiscard]] auto wheel_scroll_vertical(angle_delta_t angle_delta,
                                         const ViewConfig& view_config) -> point_fine_t;
[[nodiscard]] auto wheel_scroll_vertical_view_point(
    angle_delta_t angle_delta, const ViewConfig& view_config) -> ViewPoint;

/**
 * @brief: Calculate vertical offset from given scroll wheel rotation.
 */
[[nodiscard]] auto wheel_scroll_horizontal(angle_delta_t angle_delta,
                                           const ViewConfig& view_config) -> point_fine_t;
[[nodiscard]] auto wheel_scroll_horizontal_view_point(
    angle_delta_t angle_delta, const ViewConfig& view_config) -> ViewPoint;

/**
 * @brief: Calculate zoom and scroll offsets from wheel event.
 */
[[nodiscard]] auto wheel_scroll_zoom(const MouseWheelEvent& event,
                                     const ViewConfig& view_config)
    -> std::optional<ViewPoint>;

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
