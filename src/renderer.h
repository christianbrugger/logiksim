#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "editable_circuit.h"
#include "layout.h"
#include "schematic.h"
#include "simulation.h"

#include <blend2d.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <array>
#include <cstdint>
#include <variant>

//
// Tasks:
// * abstract Blend2d backend
// * take a scene and render it
// * store render related attributes like LED color
//

namespace logicsim {

struct RenderSettings {
    double scale {12.0};
};

auto render_background(BLContext& ctx, const RenderSettings& settings = {}) -> void;

enum class PointShape { circle, cross, plus, square, diamond, horizontal, vertical };

namespace detail {
auto set_point_style(BLContext& ctx, color_t color) -> void;
}

auto render_point(BLContext& ctx, point_t point, PointShape shape, color_t color,
                  double size, const RenderSettings& settings = {}) -> void;

auto render_points(BLContext& ctx, std::ranges::input_range auto&& points,
                   PointShape shape, color_t color, double size,
                   const RenderSettings& settings = {}) {
    detail::set_point_style(ctx, color);
    for (auto&& point : points) {
        render_point_shape(ctx, point, shape, size, settings);
    }
}

auto render_editable_circuit_caches(BLContext& ctx,
                                    const EditableCircuit& editable_circuit,
                                    const RenderSettings& settings = {}) -> void;

auto render_circuit(BLContext& ctx, const Layout& layout, const Simulation& simulation,
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
