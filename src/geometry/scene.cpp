#include "geometry/scene.h"

#include "algorithm/round.h"
#include "geometry/grid.h"
#include "geometry/rect.h"
#include "vocabulary/rect.h"
#include "vocabulary/rect_fine.h"
#include "vocabulary/view_config.h"

#include <gsl/gsl>

namespace logicsim {

// scene rect

auto get_scene_rect_fine(const ViewConfig &view_config) -> rect_fine_t {
    return rect_fine_t {
        to_grid_fine(BLPoint {0, 0}, view_config),
        to_grid_fine(BLPoint {gsl::narrow<double>(view_config.size().w),
                              gsl::narrow<double>(view_config.size().h)},
                     view_config),
    };
}

auto get_scene_rect(const ViewConfig &view_config) -> rect_t {
    return enclosing_rect(get_scene_rect_fine(view_config));
}

auto get_dirty_rect(rect_t bounding_rect, const ViewConfig &view_config) -> BLRectI {
    const auto clamp_x = [&](double x_) {
        return std::clamp(x_, 0., view_config.size().w * 1.0);
    };
    const auto clamp_y = [&](double y_) {
        return std::clamp(y_, 0., view_config.size().h * 1.0);
    };

    const auto p0 = to_context(bounding_rect.p0, view_config);
    const auto p1 = to_context(bounding_rect.p1, view_config);

    const auto padding = view_config.pixel_scale() * 0.5 + 2;

    const auto x0 = clamp_x(std::trunc(p0.x - padding));
    const auto y0 = clamp_y(std::trunc(p0.y - padding));

    const auto x1 = clamp_x(std::ceil(p1.x + padding + 1));
    const auto y1 = clamp_y(std::ceil(p1.y + padding + 1));

    return BLRectI {
        gsl::narrow<int>(x0),
        gsl::narrow<int>(y0),
        gsl::narrow<int>(x1 - x0),
        gsl::narrow<int>(y1 - y0),
    };
}

// to grid fine

auto to_grid_fine(QPointF position, const ViewConfig &config) -> point_fine_t {
    const auto scale = config.device_scale();
    const auto offset = config.offset();

    return point_fine_t {position.x() / scale, position.y() / scale} - offset;
}

auto to_grid_fine(QPoint position, const ViewConfig &config) -> point_fine_t {
    return to_grid_fine(QPointF {position}, config);
}

auto to_grid_fine(BLPoint point, const ViewConfig &config) -> point_fine_t {
    const auto scale = config.pixel_scale();
    const auto offset = config.offset();

    return point_fine_t {point.x / scale, point.y / scale} - offset;
}

// to grid

auto to_grid(QPointF position, const ViewConfig &config) -> std::optional<point_t> {
    const auto fine = to_grid_fine(position, config);

    const auto x = round(fine.x);
    const auto y = round(fine.y);

    if (is_representable(x, y)) {
        return point_t {gsl::narrow_cast<grid_t::value_type>(double {x}),
                        gsl::narrow_cast<grid_t::value_type>(double {y})};
    }
    return std::nullopt;
}

auto to_grid(QPoint position, const ViewConfig &config) -> std::optional<point_t> {
    return to_grid(QPointF {position}, config);
}

// to Qt widget / device coordinates

auto to_widget(point_fine_t position, const ViewConfig &config) -> QPoint {
    const auto scale = config.device_scale();
    const auto offset = config.offset();

    return QPoint {
        round_to<int>(double {(offset.x + position.x) * scale}),
        round_to<int>(double {(offset.y + position.y) * scale}),
    };
}

auto to_widget(point_t position, const ViewConfig &config) -> QPoint {
    return to_widget(point_fine_t {position}, config);
}

// to blend2d / pixel coordinates

auto to_context(point_fine_t position, const ViewConfig &config) -> BLPoint {
    const auto scale = config.pixel_scale();
    const auto offset = config.offset();

    return BLPoint {
        round_fast(double {(offset.x + position.x) * scale}),
        round_fast(double {(offset.y + position.y) * scale}),
    };
}

auto to_context(point_t position, const ViewConfig &config) -> BLPoint {
    return to_context(point_fine_t {position}, config);
}

auto to_context(grid_fine_t length, const ViewConfig &config) -> double {
    const auto scale = config.pixel_scale();
    return round_fast(double {length} * scale);
}

auto to_context(grid_t length, const ViewConfig &config) -> double {
    return to_context(grid_fine_t {length}, config);
}

auto to_context_unrounded(grid_fine_t length, const ViewConfig &config) -> double {
    const auto scale = config.pixel_scale();
    return double {length} * scale;
}

}  // namespace logicsim