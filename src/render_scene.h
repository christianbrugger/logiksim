#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "circuit.h"
#include "geometry.h"
#include "line_tree.h"
#include "simulation.h"

#include <blend2d.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <array>
#include <cstdint>
#include <variant>

namespace logicsim {

// TODO remove
using wire_index_t = uint16_t;

class SimulationScene {
   public:
    [[nodiscard]] explicit SimulationScene(const Simulation& simulation) noexcept;

    auto render_scene(BLContext& ctx, bool render_background = true) const -> void;

    auto set_position(Circuit::ConstElement element, point2d_t position) -> void;
    auto set_line_tree(Circuit::ConstElement element, LineTree&& line_tree) -> void;

   private:
    struct DrawData {
        LineTree line_tree {};

        point2d_t position {0, 0};
        int8_t orientation {0};
    };

    auto get_data(Circuit::ConstElement element) -> DrawData&;
    auto get_data(Circuit::ConstElement element) const -> const DrawData&;

    auto draw_background(BLContext& ctx) const -> void;
    auto draw_wire(BLContext& ctx, Circuit::ConstElement element) const -> void;
    auto draw_standard_element(BLContext& ctx, Circuit::ConstElement element) const
        -> void;

   private:
    gsl::not_null<const Simulation*> simulation_;
    std::vector<DrawData> draw_data_vector_ {};
};

struct BenchmarkScene {
   public:
    Circuit circuit {};
    Simulation simulation {circuit};
    SimulationScene renderer {simulation};
};

auto fill_line_scene(BenchmarkScene& scene, int n_lines = 100) -> int64_t;

auto benchmark_line_renderer(int n_lines = 100, bool save_image = true) -> int64_t;

}  // namespace logicsim

#endif
