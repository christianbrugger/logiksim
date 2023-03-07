#ifndef LOGIKSIM_SCENE_H
#define LOGIKSIM_SCENE_H

#include "vocabulary.h"

#include <blend2d.h>

#include <QPoint>

#include <optional>

namespace logicsim {

struct ViewConfig {
    point_fine_t offset {};  // in fine grid points
    double scale {12.};
};

auto is_representable(int x, int y) -> bool;
auto is_representable(double x, double y) -> bool;

// to grid fine
auto to_grid_fine(double x, double y, ViewConfig config) -> point_fine_t;
auto to_grid_fine(QPointF position, ViewConfig config) -> point_fine_t;
auto to_grid_fine(QPoint position, ViewConfig config) -> point_fine_t;

// to grid
auto to_grid(double x, double y, ViewConfig config) -> std::optional<point_t>;
auto to_grid(QPointF position, ViewConfig config) -> std::optional<point_t>;
auto to_grid(QPoint position, ViewConfig config) -> std::optional<point_t>;

// to Qt widget coordinates
auto to_widget(point_t position, ViewConfig config) -> QPoint;
auto to_widget(point_fine_t position, ViewConfig config) -> QPoint;

// to blend2d context coordinates
auto to_context(point_t position, ViewConfig config) -> BLPoint;
auto to_context(point_fine_t position, ViewConfig config) -> BLPoint;

auto to_context(grid_t length, ViewConfig config) -> double;
auto to_context(double length, ViewConfig config) -> double;

}  // namespace logicsim

#endif
