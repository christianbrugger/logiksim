#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "circuit.h"
#include "simulation.h"

#include <blend2d.h>
#include <folly/small_vector.h>

#include <array>
#include <cstdint>

namespace logicsim {

//           / --- c
//  a ---- b
//           \ --- d
//

struct point2d_t {
    int16_t x;
    int16_t y;
};

using point_vector_t = folly::small_vector<point2d_t, 2, uint16_t>;
using pair_vector_t = folly::small_vector<uint16_t, 4, uint16_t>;

static_assert(sizeof(point_vector_t) == 10);
static_assert(sizeof(pair_vector_t) == 10);

/* For one element in the circuit. */
struct DrawAttribute {
    point_vector_t points;
    pair_vector_t indices;

    int8_t orientation;
};

using attribute_vector_t = std::vector<DrawAttribute>;

auto render_scene(BLContext& ctx, const Simulation& simulation,
                  const attribute_vector_t& attributes) -> void;

}  // namespace logicsim

#endif
