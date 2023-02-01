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
    // TODO remove
    auto set_line_tree(Circuit::ConstElement element, std::vector<point2d_t> points,
                       std::vector<wire_index_t> indices) -> void;
    auto set_line_tree(Circuit::ConstElement element, LineTree&& line_tree) -> void;

   private:
    using policy = folly::small_vector_policy::policy_size_type<uint16_t>;
    using point_vector_t = folly::small_vector<point2d_t, 2, policy>;
    using index_vector_t = folly::small_vector<wire_index_t, 4, policy>;

    static_assert(sizeof(point_vector_t) == 10);
    static_assert(sizeof(index_vector_t) == 10);

    struct DrawData {
        LineTree line_tree {};

        point_vector_t points {};
        index_vector_t indices {};

        point2d_t position {0, 0};
        int8_t orientation {0};
    };

   public:  // TODO make private again
    auto get_data(Circuit::ConstElement element) -> DrawData&;
    auto get_data(Circuit::ConstElement element) const -> const DrawData&;

   private:
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

// TODO remove use_new
auto fill_line_scene(BenchmarkScene& scene, int n_lines = 100, bool use_new = true)
    -> int64_t;

auto benchmark_line_renderer(int n_lines = 100, bool save_image = true) -> int64_t;

}  // namespace logicsim

#endif
