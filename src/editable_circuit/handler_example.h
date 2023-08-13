#ifndef LOGIKSIM_EDITABLE_CIRCUIT_HANDLER_EXAMPLE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_HANDLER_EXAMPLE_H

#include "editable_circuit/handler.h"
#include "random.h"

namespace logicsim::editable_circuit::examples {

auto add_many_wires(Rng& rng, State state, bool random_modes, int max_tries = 100'000)
    -> void;

auto add_many_wires_and_buttons(Rng& rng, State state, bool random_modes,
                                int max_tries = 100'000) -> void;

}  // namespace logicsim::editable_circuit::examples

#endif
