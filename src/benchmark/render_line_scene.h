#ifndef LOGICSIM_BENCHMARK_RENDER_LINE_SCENE_H
#define LOGICSIM_BENCHMARK_RENDER_LINE_SCENE_H

#include "layout.h"
#include "schematic.h"
#include "simulation.h"
#include "vocabulary/simulation_setting.h"

#include <cstdint>

namespace logicsim {

struct BenchmarkScene {
   public:
    Schematic schematic {default_wire_delay_per_distance()};
    Layout layout {};
    Simulation simulation {schematic};
};

auto fill_line_scene(BenchmarkScene& scene, int n_lines = 100) -> int64_t;

auto benchmark_line_renderer(int n_lines = 100, bool save_image = true) -> int64_t;

}  // namespace logicsim

#endif
