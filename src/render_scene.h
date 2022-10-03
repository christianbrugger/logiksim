#ifndef LOGIKSIM_RENDER_SCENE_H
#define LOGIKSIM_RENDER_SCENE_H

#include "circuit.h"
#include "simulation.h"

#include <boost/container/small_vector.hpp>
#include <blend2d.h>

#include <array>
#include <cstdint>

namespace logicsim {

    struct SimulationResult {
        logic_vector_t input_values;
        logic_vector_t output_values;
    };

    using point2d_t = std::array<double, 2>;
    using line_tree_t = boost::container::small_vector<point2d_t, 2>;

    /* Draw attributes for one element in the graph. */
    struct DrawAttributes {
        line_tree_t line_tree;
        point2d_t position;

        int8_t orientation;
    };

    using attribute_vector_t = std::vector<DrawAttributes>;

	void render_scene(
        BLContext& ctx, 
        const CircuitGraph& graph, 
        const SimulationResult& simulation, 
        const attribute_vector_t &attributes
    );

}

#endif
