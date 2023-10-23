#ifndef LOGICSIM_BENCHMARK_RENDER_LINE_SCENE_H
#define LOGICSIM_BENCHMARK_RENDER_LINE_SCENE_H

#include "layout.h"
#include "schematic.h"
#include "schematic_old.h"
#include "simulation.h"
#include "vocabulary/simulation_setting.h"

#include <cstdint>

namespace logicsim {

struct SimulatedLineScene {
    Layout layout;
    Simulation simulation;
    int64_t total_wire_length_sum;
};

auto fill_line_scene(int n_lines = 100) -> SimulatedLineScene;

auto benchmark_line_renderer(int n_lines = 100, bool save_image = true) -> int64_t;

}  // namespace logicsim

#endif
