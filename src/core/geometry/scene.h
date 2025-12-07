#ifndef LOGICSIM_GEOMETRY_SCENE_H
#define LOGICSIM_GEOMETRY_SCENE_H

#include <blend2d/blend2d.h>

#include <optional>

namespace logicsim {

struct ViewConfig;

struct grid_fine_t;
struct grid_t;
struct point_t;
struct point_fine_t;
struct rect_fine_t;
struct rect_t;
struct point_device_t;
struct point_device_fine_t;
struct size_device_t;

// scene rect
[[nodiscard]] auto get_scene_rect_fine(const ViewConfig& view_config) -> rect_fine_t;
[[nodiscard]] auto get_scene_rect(const ViewConfig& view_config) -> rect_t;
/**
 * @brief: pixels that need to be rendered within bounding rect and view.
 */
[[nodiscard]] auto get_dirty_rect(rect_t bounding_rect,
                                  const ViewConfig& view_config) -> BLRectI;

// to grid fine
[[nodiscard]] auto to_grid_fine(point_device_fine_t position,
                                const ViewConfig& config) -> point_fine_t;
[[nodiscard]] auto to_grid_fine(point_device_t position,
                                const ViewConfig& config) -> point_fine_t;
[[nodiscard]] auto to_grid_fine(BLPoint point, const ViewConfig& config) -> point_fine_t;

// to grid
[[nodiscard]] auto to_grid(point_device_fine_t position,
                           const ViewConfig& config) -> std::optional<point_t>;
[[nodiscard]] auto to_grid(point_device_t position,
                           const ViewConfig& config) -> std::optional<point_t>;

[[nodiscard]] auto to_closest_grid_position(point_device_fine_t position,
                                            size_device_t widget_size,
                                            const ViewConfig& config) -> point_t;

// to device coordinates used in the GUI / Qt
[[nodiscard]] auto to_device(point_fine_t position,
                             const ViewConfig& config) -> point_device_t;
[[nodiscard]] auto to_device(point_t position,
                             const ViewConfig& config) -> point_device_t;

// to blend2d / pixel coordinates
[[nodiscard]] auto to_context(point_fine_t position, const ViewConfig& config) -> BLPoint;
[[nodiscard]] auto to_context(point_t position, const ViewConfig& config) -> BLPoint;

[[nodiscard]] auto to_context(point_device_fine_t position,
                              const ViewConfig& config) -> BLPoint;
[[nodiscard]] auto to_context(point_device_t position,
                              const ViewConfig& config) -> BLPoint;

[[nodiscard]] auto to_context(grid_fine_t length, const ViewConfig& config) -> double;
[[nodiscard]] auto to_context(grid_t length, const ViewConfig& config) -> double;

[[nodiscard]] auto to_context_unrounded(grid_fine_t length,
                                        const ViewConfig& config) -> double;

}  // namespace logicsim

#endif
