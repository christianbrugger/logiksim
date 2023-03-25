#include "scene.h"

#include <gsl/gsl>

#include <cassert>
#include <cmath>

namespace logicsim {

auto ViewConfig::format() const -> std::string {
    return fmt::format(
        "RenderSettings(\n"
        "  offset = {},\n"
        "  pixel_scale = {},\n"
        "  device_scale = {},\n"
        "  device_pixel_ratio = {})",
        offset(), pixel_scale(), device_scale(), device_pixel_ratio());
}

auto ViewConfig::offset() const noexcept -> point_fine_t {
    return offset_;
}

auto ViewConfig::pixel_scale() const noexcept -> double {
    return pixel_scale_;
}

auto ViewConfig::device_scale() const noexcept -> double {
    return device_scale_;
}

auto ViewConfig::device_pixel_ratio() const noexcept -> double {
    return device_pixel_ratio_;
}

auto ViewConfig::set_offset(point_fine_t offset) -> void {
    offset_ = offset;
}

auto ViewConfig::set_device_scale(double device_scale) -> void {
    device_scale_ = device_scale;
    update_pixel_scale();
}

auto ViewConfig::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    device_pixel_ratio_ = device_pixel_ratio;
    update_pixel_scale();
}

auto ViewConfig::update_pixel_scale() -> void {
    pixel_scale_ = device_scale_ * device_pixel_ratio_;
}

//
// Free functions
//

auto is_representable(int x, int y) -> bool {
    return (grid_t::min() <= x && x <= grid_t::max())
           && (grid_t::min() <= y && y <= grid_t::max());
}

auto is_representable(double x, double y) -> bool {
    return (grid_t::min() <= x && x <= grid_t::max())
           && (grid_t::min() <= y && y <= grid_t::max());
}

// device to grid fine

auto to_grid_fine(double x, double y, const ViewConfig &config) -> point_fine_t {
    const auto scale = config.device_scale();
    const auto offset = config.offset();

    return {
        x / scale - offset.x,
        y / scale - offset.y,
    };
}

auto to_grid_fine(QPointF position, const ViewConfig &config) -> point_fine_t {
    return to_grid_fine(position.x(), position.y(), config);
}

auto to_grid_fine(QPoint position, const ViewConfig &config) -> point_fine_t {
    return to_grid_fine(position.x(), position.y(), config);
}

// device to grid

auto to_grid(double x_, double y_, const ViewConfig &config) -> std::optional<point_t> {
    const auto fine = to_grid_fine(x_, y_, config);

    const auto x = round_fast(fine.x);
    const auto y = round_fast(fine.y);

    if (is_representable(x, y)) {
        return point_t {gsl::narrow_cast<grid_t::value_type>(x),
                        gsl::narrow_cast<grid_t::value_type>(y)};
    }
    return std::nullopt;
}

auto to_grid(QPointF position, const ViewConfig &config) -> std::optional<point_t> {
    return to_grid(position.x(), position.y(), config);
}

auto to_grid(QPoint position, const ViewConfig &config) -> std::optional<point_t> {
    return to_grid(position.x(), position.y(), config);
}

// to Qt widget / device coordinates

auto to_widget(point_fine_t position, const ViewConfig &config) -> QPoint {
    const auto scale = config.device_scale();
    const auto offset = config.offset();

    return QPoint {
        round_to<int>((offset.x + position.x) * scale),
        round_to<int>((offset.y + position.y) * scale),
    };
}

auto to_widget(point_t position, const ViewConfig &config) -> QPoint {
    return to_widget(static_cast<point_fine_t>(position), config);
}

// to blend2d / pixel coordinates

auto to_context(point_fine_t position, const ViewConfig &config) -> BLPoint {
    const auto scale = config.pixel_scale();
    const auto offset = config.offset();

    return BLPoint {
        round_fast((offset.x + position.x) * scale),
        round_fast((offset.y + position.y) * scale),
    };
}

auto to_context(point_t position, const ViewConfig &config) -> BLPoint {
    return to_context(static_cast<point_fine_t>(position), config);
}

auto to_context(double length, const ViewConfig &config) -> double {
    const auto scale = config.pixel_scale();
    return round_fast(length * scale);
}

auto to_context(grid_t length, const ViewConfig &config) -> double {
    return to_context(length.value, config);
}

}  // namespace logicsim