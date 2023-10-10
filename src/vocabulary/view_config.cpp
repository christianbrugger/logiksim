#include "vocabulary/view_config.h"

namespace logicsim {

ViewConfig::ViewConfig() {
    update();
}

auto ViewConfig::format() const -> std::string {
    return fmt::format(
        "OldRenderSettings(\n"
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
    scale_device_ = device_scale;
    update();
}

auto ViewConfig::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    device_ratio_px_ = device_pixel_ratio;
    update();
}

auto ViewConfig::set_size(BLSizeI size) -> void {
    size_px_ = size;
}

auto ViewConfig::stroke_width() const noexcept -> int {
    return stroke_width_px_;
}

auto ViewConfig::line_cross_width() const noexcept -> int {
    return line_cross_width_px_;
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
