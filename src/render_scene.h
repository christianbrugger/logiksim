#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "circuit.h"
#include "geometry.h"
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

    auto render_scene(BLContext& ctx) const -> void;

    auto set_position(Circuit::ConstElement element, point2d_t position) -> void;
    auto set_line_tree(Circuit::ConstElement element, std::vector<point2d_t> points,
                       std::vector<wire_index_t> indices) -> void;

   private:
    using point_vector_t = folly::small_vector<point2d_t, 2, uint16_t>;
    using index_vector_t = folly::small_vector<wire_index_t, 4, uint16_t>;

    static_assert(sizeof(point_vector_t) == 10);
    static_assert(sizeof(index_vector_t) == 10);

    struct DrawData {
        point_vector_t points {};
        index_vector_t indices {};

        point2d_t position {0, 0};
        int8_t orientation {0};
    };

   private:
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

auto benchmark_line_renderer(int n_lines = 100) -> int64_t;

}  // namespace logicsim

#endif
