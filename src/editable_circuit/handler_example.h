#ifndef LOGIKSIM_EDITABLE_CIRCUIT_HANDLER_EXAMPLE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_HANDLER_EXAMPLE_H

#include "editable_circuit/handler.h"
#include "random.h"

namespace logicsim::editable_circuit::examples {

auto add_many_wires(Rng& rng, State state, bool random_modes, int max_tries = 100'000)
    -> void;

struct WiresButtonsParams {
    bool random_modes {false};
    int tries_start {5};
    int tries_end {100};
    grid_t grid_start {5};
    grid_t grid_end {10};
    grid_t max_length {50};
};

auto add_many_wires_and_buttons(Rng& rng, State state, WiresButtonsParams params = {})
    -> void;

}  // namespace logicsim::editable_circuit::examples

#endif
