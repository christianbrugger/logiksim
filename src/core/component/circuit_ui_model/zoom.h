#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_ZOOM_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_ZOOM_H

namespace logicsim {

struct point_device_fine_t;
struct ViewPoint;
struct ViewConfig;

namespace circuit_ui_model {

/**
 * @brief: Calculate the zoomed view point at given position.
 */
[[nodiscard]] auto zoom(ViewConfig view_config, double steps,
                        point_device_fine_t center) -> ViewPoint;

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
