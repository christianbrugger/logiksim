#ifndef LOGIKSIM_SCENE_H
#define LOGIKSIM_SCENE_H

#include "format.h"
#include "vocabulary.h"

#include <blend2d.h>

#include <QPoint>

#include <optional>

namespace logicsim {

class ViewConfig {
   public:
    auto format() const -> std::string;

    auto offset() const noexcept -> point_fine_t;
    auto pixel_scale() const noexcept -> double;
    auto device_scale() const noexcept -> double;
    auto device_pixel_ratio() const noexcept -> double;

    auto set_offset(point_fine_t offset) -> void;
    auto set_device_scale(double device_scale) -> void;
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;

   private:
    auto update_pixel_scale() -> void;

    point_fine_t offset_ {};  // in fine grid points
    double device_pixel_ratio_ {1.};
    double device_scale_ {12.};
    double pixel_scale_ {12.};  // updated internally
};

[[nodiscard]] auto is_representable(int x, int y) -> bool;
[[nodiscard]] auto is_representable(double x, double y) -> bool;
[[nodiscard]] auto is_representable(point_t point, int dx, int dy) -> bool;
[[nodiscard]] auto is_representable(line_t point, int dx, int dy) -> bool;
[[nodiscard]] auto is_representable(ordered_line_t point, int dx, int dy) -> bool;

// device to grid fine
[[nodiscard]] auto to_grid_fine(double x, double y, const ViewConfig& config)
    -> point_fine_t;
[[nodiscard]] auto to_grid_fine(QPointF position, const ViewConfig& config)
    -> point_fine_t;
[[nodiscard]] auto to_grid_fine(QPoint position, const ViewConfig& config)
    -> point_fine_t;

// device to grid
[[nodiscard]] auto to_grid(double x, double y, const ViewConfig& config)
    -> std::optional<point_t>;
[[nodiscard]] auto to_grid(QPointF position, const ViewConfig& config)
    -> std::optional<point_t>;
[[nodiscard]] auto to_grid(QPoint position, const ViewConfig& config)
    -> std::optional<point_t>;

// to Qt widget / device coordinates
[[nodiscard]] auto to_widget(point_t position, const ViewConfig& config) -> QPoint;
[[nodiscard]] auto to_widget(point_fine_t position, const ViewConfig& config) -> QPoint;

// to blend2d / pixel coordinates
[[nodiscard]] auto to_context(point_t position, const ViewConfig& config) -> BLPoint;
[[nodiscard]] auto to_context(point_fine_t position, const ViewConfig& config) -> BLPoint;

[[nodiscard]] auto to_context(grid_t length, const ViewConfig& config) -> double;
[[nodiscard]] auto to_context(double length, const ViewConfig& config) -> double;

// from blend2d / pixel coordinates
[[nodiscard]] auto from_context_fine(BLPoint point, const ViewConfig& config)
    -> point_fine_t;

}  // namespace logicsim

#endif
