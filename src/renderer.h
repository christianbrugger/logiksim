#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "circuit_layout.h"
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
    bool render_background {true};
};

auto render_circuit(BLContext& ctx, const CircuitLayout& layout,
                    const Simulation& simulation,
                    const RenderSettings& settings = RenderSettings {}) -> void;

struct BenchmarkScene {
   public:
    Circuit circuit {};
    CircuitLayout layout {};
    Simulation simulation {circuit};
};

auto fill_line_scene(BenchmarkScene& scene, int n_lines = 100) -> int64_t;

auto benchmark_line_renderer(int n_lines = 100, bool save_image = true) -> int64_t;

}  // namespace logicsim

#endif
