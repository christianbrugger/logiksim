#include "vocabulary/view_config.h"

#include <exception>

namespace logicsim {

auto ViewPoint::format() const -> std::string {
    return fmt::format(
        "ViewPoint(\n"
        "  offset = {} grid,\n"
        "  device_scale = {} coord,\n"
        ")",
        offset, device_scale);
}

//
// View Config
//

ViewConfig::ViewConfig() {
    update();
}

ViewConfig::ViewConfig(BLSizeI size_px) : size_px_{size_px} {
    update();
}

auto ViewConfig::format() const -> std::string {
    return fmt::format(
        "ViewConfig(\n"
        "  offset = {} grid,\n"
        "  size = {} x {} px,\n"
        "  pixel_scale = {} px,\n"
        "  device_scale = {} coord,\n"
        "  device_pixel_ratio = {} px)",
        offset(), size().w, size().h, pixel_scale(), device_scale(),
        device_pixel_ratio());
}

auto ViewConfig::offset() const noexcept -> point_fine_t {
    return offset_;
}

auto ViewConfig::pixel_scale() const noexcept -> double {
    return scale_px_;
}

auto ViewConfig::device_scale() const noexcept -> double {
    return scale_device_;
}

auto ViewConfig::device_pixel_ratio() const noexcept -> double {
    return device_ratio_px_;
}

auto ViewConfig::size() const noexcept -> BLSizeI {
    return size_px_;
}

auto ViewConfig::set_offset(point_fine_t offset) -> void {
    offset_ = offset;
}

auto ViewConfig::set_device_scale(double device_scale) -> void {
    if (device_scale <= 0) [[unlikely]] {
        throw std::runtime_error("device_scale needs to be positive");
    }
    scale_device_ = device_scale;
    update();
}

auto ViewConfig::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    if (device_pixel_ratio <= 0) [[unlikely]] {
        throw std::runtime_error("device_pixel_ratio needs to be positive");
    }
    device_ratio_px_ = device_pixel_ratio;
    update();
}

auto ViewConfig::set_size(BLSizeI size) -> void {
    if (size.w < int {0} || size.h < int {0}) [[unlikely]] {
        throw std::runtime_error("size needs to be positive or zero");
    }
    size_px_ = size;
}

auto ViewConfig::stroke_width() const noexcept -> int {
    return stroke_width_px_;
}

auto ViewConfig::line_cross_width() const noexcept -> int {
    return line_cross_width_px_;
}

auto ViewConfig::view_point() const noexcept -> ViewPoint {
    return ViewPoint {
        .offset = offset(),
        .device_scale = device_scale(),
    };
}

auto ViewConfig::set_view_point(const ViewPoint &view_point) -> void {
    set_device_scale(view_point.device_scale);
    set_offset(view_point.offset);
}

auto ViewConfig::update() -> void {
    // pixel scale
    {
        scale_px_ = scale_device_ * device_ratio_px_;  //
    }

    // stroke width
    {
        constexpr static auto stepping = 16;  // 12
        const auto scale = pixel_scale();
        stroke_width_px_ = std::max(1, static_cast<int>(scale / stepping));
    }

    // line cross width
    {
        constexpr static auto stepping = 8;
        const auto scale = pixel_scale();
        line_cross_width_px_ = std::max(1, static_cast<int>(scale / stepping));
    }
}

}  // namespace logicsim
