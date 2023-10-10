#ifndef LOGICSIM_VOCABULARY_VIEW_CONFIG_H
#define LOGICSIM_VOCABULARY_VIEW_CONFIG_H

#include "format/struct.h"
#include "vocabulary/point_fine.h"

#include <blend2d.h>

#include <compare>
#include <string>

namespace logicsim {

struct ViewConfig {
   public:
    ViewConfig();

    auto set_offset(point_fine_t offset) -> void;
    auto set_device_scale(double device_scale) -> void;
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;
    auto set_size(BLSizeI size) -> void;

    // TODO maybe rename some
    [[nodiscard]] auto offset() const noexcept -> point_fine_t;
    [[nodiscard]] auto pixel_scale() const noexcept -> double;
    [[nodiscard]] auto device_scale() const noexcept -> double;
    [[nodiscard]] auto device_pixel_ratio() const noexcept -> double;
    [[nodiscard]] auto size() const noexcept -> BLSizeI;

    [[nodiscard]] auto stroke_width() const noexcept -> int;
    [[nodiscard]] auto line_cross_width() const noexcept -> int;

    [[nodiscard]] auto operator==(const ViewConfig& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const ViewConfig& other) const = default;

    [[nodiscard]] auto format() const -> std::string;

   private:
    auto update() -> void;

    point_fine_t offset_ {};
    double device_ratio_px_ {1.};  // pixels of one device coordinates
    double scale_device_ {18.};    // distance of grid in device coordinates
    BLSizeI size_px_ {};           // view size in pixels

    // updated internally
    double scale_px_ {};          // distance of grid in pixel coordinates
    int stroke_width_px_ {};      // stroke width in pixels
    int line_cross_width_px_ {};  // stroke width in pixels
};

}  // namespace logicsim

#endif
