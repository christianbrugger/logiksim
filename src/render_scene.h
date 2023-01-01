#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "circuit.h"
#include "simulation.h"

#include <blend2d.h>
#include <boost/container/small_vector.hpp>

#include <array>
#include <cstdint>

namespace logicsim {

using point2d_t = std::array<double, 2>;
using line_tree_t = boost::container::small_vector<point2d_t, 2>;

/* Draw attributes for one element in the circuit. */
struct DrawAttributes {
    line_tree_t line_tree;
    point2d_t position;

    int8_t orientation;
};

using attribute_vector_t = std::vector<DrawAttributes>;

auto render_scene(BLContext& ctx, const Simulation& simulation,
                  const attribute_vector_t& attributes) -> void;

}  // namespace logicsim

#endif
