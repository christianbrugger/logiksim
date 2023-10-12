#ifndef LOGICSIM_GEOMETRY_LAYOUT_CALCULATION_H
#define LOGICSIM_GEOMETRY_LAYOUT_CALCULATION_H

#include "vocabulary/orientation.h"

struct BLPoint;

namespace logicsim {

struct grid_fine_t;
struct point_t;
struct point_fine_t;
struct rect_t;

[[nodiscard]] auto connector_point(point_t position, orientation_t orientation,
                                   grid_fine_t offset) -> point_fine_t;

[[nodiscard]] auto connector_point(BLPoint position, orientation_t orientation,
                                   double offset) -> BLPoint;

[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_t offset) -> point_t;

[[nodiscard]] auto transform(point_t element_position, orientation_t orientation,
                             point_fine_t offset) -> point_fine_t;

[[nodiscard]] auto transform(orientation_t element_orientation, orientation_t connector)
    -> orientation_t;

[[nodiscard]] auto transform(point_t position, orientation_t orientation, point_t p0,
                             point_t p1) -> rect_t;

}  // namespace logicsim

#endif
