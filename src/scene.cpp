#include "scene.h"

#include "geometry.h"

#include <gsl/gsl>

#include <algorithm>
#include <cassert>
#include <cmath>

namespace logicsim {

ViewConfig::ViewConfig() {
    update();
}

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

auto ViewConfig::width() const noexcept -> int {
    return width_;
}

auto ViewConfig::height() const noexcept -> int {
    return height_;
}

auto ViewConfig::set_offset(point_fine_t offset) -> void {
    offset_ = offset;
}

auto ViewConfig::set_device_scale(double device_scale) -> void {
    device_scale_ = device_scale;
    update();
}

auto ViewConfig::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    device_pixel_ratio_ = device_pixel_ratio;
    update();
}

auto ViewConfig::set_size(int width, int height) -> void {
    width_ = width;
    height_ = height;
}

auto ViewConfig::stroke_width() const noexcept -> int {
    return stroke_width_;
}

auto ViewConfig::line_cross_width() const noexcept -> int {
    return line_cross_width_;
}

auto ViewConfig::update() -> void {
    // pixel scale
    {
        pixel_scale_ = device_scale_ * device_pixel_ratio_;  //
    }

    // stroke width
    {
        constexpr static auto stepping = 16;  // 12
        const auto scale = pixel_scale();
        stroke_width_ = std::max(1, static_cast<int>(scale / stepping));
    }

    // line cross width
    {
        constexpr static auto stepping = 8;
        const auto scale = pixel_scale();
        line_cross_width_ = std::max(1, static_cast<int>(scale / stepping));
    }
}

//
// Free functions
//

auto is_representable(int x, int y) -> bool {
    return (grid_t::min() <= x && x <= grid_t::max()) &&
           (grid_t::min() <= y && y <= grid_t::max());
}

auto is_representable(double x, double y) -> bool {
    return (grid_t::min() <= x && x <= grid_t::max()) &&
           (grid_t::min() <= y && y <= grid_t::max());
}

auto is_representable(point_t point, int dx, int dy) -> bool {
    return is_representable(point.x.value + dx, point.y.value + dy);
}

auto is_representable(line_t line, int dx, int dy) -> bool {
    return is_representable(line.p0, dx, dy) && is_representable(line.p1, dx, dy);
}

auto is_representable(ordered_line_t line, int dx, int dy) -> bool {
    return is_representable(line_t {line}, dx, dy);
}

// scene rect

auto get_scene_rect_fine(const ViewConfig &view_config) -> rect_fine_t {
    return rect_fine_t {
        from_context_fine(BLPoint {0, 0}, view_config),
        from_context_fine(BLPoint {gsl::narrow<double>(view_config.width()),
                                   gsl::narrow<double>(view_config.height())},
                          view_config),
    };
}

auto get_scene_rect(const ViewConfig &view_config) -> rect_t {
    return to_enclosing_rect(get_scene_rect_fine(view_config));
}

// device to grid fine

auto to_grid_fine(double x, double y, const ViewConfig &config) -> point_fine_t {
    const auto scale = config.device_scale();
    const auto offset = config.offset();

    return point_fine_t {
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
    return to_widget(point_fine_t {position}, config);
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
    return to_context(point_fine_t {position}, config);
}

auto to_context(double length, const ViewConfig &config) -> double {
    const auto scale = config.pixel_scale();
    return round_fast(length * scale);
}

auto to_context(grid_t length, const ViewConfig &config) -> double {
    return to_context(length.value, config);
}

// from blend2d / pixel coordinates

auto from_context_fine(BLPoint point, const ViewConfig &config) -> point_fine_t {
    const auto scale = config.pixel_scale();
    const auto offset = config.offset();

    return point_fine_t {
        point.x / scale - offset.x,
        point.y / scale - offset.y,
    };
}

}  // namespace logicsim