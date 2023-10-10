#ifndef LOGICSIM_VOCABULARY_VIEW_CONFIG_H
#define LOGICSIM_VOCABULARY_VIEW_CONFIG_H

#include "format/struct.h"
#include "vocabulary/point_fine.h"

#include <blend2d.h>

#include <compare>
#include <string>

namespace logicsim {

/**
 * brief: Defines the rendered area of the grid and how it relates to
 *        device coordinates and real pixels.
 */
struct ViewConfig {
   public:
    ViewConfig();

    /**
     * brief: Set the offset of the viewed area in grid coordinates.
     */
    auto set_offset(point_fine_t offset) -> void;

    /**
     * brief: Set how large two grid points appear on screen.
     *
     * Note this is in device coordinates.
     */
    auto set_device_scale(double device_scale) -> void;

    /**
     * brief:  Set how many pixels a device coordinate occupies.
     *
     * Note this is to support high DPI screens.
     */
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;

    /**
     * brief: Set the size of the viewed area in pixels.
     *
     * Note that objects are clipped outside of this size.
     */
    auto set_size(BLSizeI size) -> void;

    /**
     * brief: The offset of the viewed area in grid coordinates.
     */
    [[nodiscard]] auto offset() const noexcept -> point_fine_t;

    /**
     * brief: How large the grid is in pixels.
     */
    [[nodiscard]] auto pixel_scale() const noexcept -> double;

    /**
     * brief: How large two grid points appear on screen.
     */
    [[nodiscard]] auto device_scale() const noexcept -> double;

    /**
     * brief: How many pixels a device coordinate occupies.
     */
    [[nodiscard]] auto device_pixel_ratio() const noexcept -> double;

    /**
     * brief: Get size of drawable area in pixels.
     */
    [[nodiscard]] auto size() const noexcept -> BLSizeI;

    /**
     * brief: Width of standard strokes of rects, or lines in pixels.
     */
    [[nodiscard]] auto stroke_width() const noexcept -> int;

    /**
     * brief: Width of line cross point in pixels.
     */
    [[nodiscard]] auto line_cross_width() const noexcept -> int;

    [[nodiscard]] auto operator==(const ViewConfig& other) const -> bool = default;
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
