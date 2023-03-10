#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "editable_circuit.h"
#include "layout.h"
#include "scene.h"
#include "schematic.h"
#include "simulation.h"

#include <blend2d.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <array>
#include <cstdint>
#include <variant>

namespace logicsim {

//
// primitives
//

struct RenderSettings {
    ViewConfig view_config {};

    int background_grid_min_distance {10};  // pixel
};

auto stroke_width(const RenderSettings& settings) -> int;
auto stroke_offset(int stroke_width) -> double;
auto stroke_offset(const RenderSettings& settings) -> double;

enum class PointShape {
    circle,
    cross,
    plus,
    square,
    full_square,
    diamond,
    horizontal,
    vertical
};

auto render_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                  double size, const RenderSettings& settings) -> void;

auto render_points(BLContext& ctx, std::ranges::input_range auto&& points,
                   PointShape shape, color_t color, double size,
                   const RenderSettings& settings) {
    for (auto&& point : points) {
        render_point(ctx, point, shape, color, size, settings);
    }
}

auto render_arrow(BLContext& ctx, point_t point, color_t color, orientation_t orientation,
                  double size, const RenderSettings& settings) -> void;
auto render_input_marker(BLContext& ctx, point_t point, color_t color,
                         orientation_t orientation, double size,
                         const RenderSettings& settings) -> void;

//
// scenes
//

auto render_background(BLContext& ctx, const RenderSettings& settings = {}) -> void;

auto render_editable_circuit_connection_cache(BLContext& ctx,
                                              const EditableCircuit& editable_circuit,
                                              const RenderSettings& settings) -> void;
auto render_editable_circuit_collision_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void;
auto render_editable_circuit_selection_cache(BLContext& ctx,
                                             const EditableCircuit& editable_circuit,
                                             const RenderSettings& settings) -> void;

using selection_mask_t = boost::container::vector<bool>;

auto render_circuit(BLContext& ctx, const Schematic& schematic, const Layout& layout,
                    const Simulation* simulation = nullptr,
                    const selection_mask_t& selection_mask = {},
                    const RenderSettings& settings = {}) -> void;

struct BenchmarkScene {
   public:
    Schematic schematic {};
    Layout layout {};
    Simulation simulation {schematic};
};

auto fill_line_scene(BenchmarkScene& scene, int n_lines = 100) -> int64_t;

auto benchmark_line_renderer(int n_lines = 100, bool save_image = true) -> int64_t;

}  // namespace logicsim

#endif
