#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "editable_circuit/selection.h"
#include "format.h"
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

class Selection;

//
// primitives
//

struct RenderSettings {
    ViewConfig view_config {};

    int background_grid_min_distance {10};  // device pixels

    auto format() const -> std::string;
};

auto stroke_width(const RenderSettings& settings) -> int;
auto stroke_offset(int stroke_width) -> double;
auto stroke_offset(const RenderSettings& settings) -> double;

enum class PointShape {
    circle,
    full_circle,
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

class EditableCircuit;

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

// TODO better grouping, group RenderSettings and BLContext, Circuit struct
struct render_args_t {
    const Layout& layout;
    const Schematic* schematic {nullptr};
    const Simulation* simulation {nullptr};
    const selection_mask_t& selection_mask {};
    const Selection& selection {};
    const RenderSettings& settings {};
};

auto render_circuit(BLContext& ctx, render_args_t args) -> void;

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
