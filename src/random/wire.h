#ifndef LOGICSIM_RANDOM_WIRE_H
#define LOGICSIM_RANDOM_WIRE_H

#include "random/generator.h"
#include "vocabulary/grid.h"

namespace logicsim {

class EditableCircuit;

namespace editable_circuit {

auto add_random_wire(Rng& rng, EditableCircuit& editable_circuit, grid_t min, grid_t max,
                     grid_t max_length, bool random_modes) -> void;

auto add_random_button(Rng& rng, EditableCircuit& editable_circuit, grid_t min,
                       grid_t max, bool random_modes) -> void;

auto add_many_wires(Rng& rng, EditableCircuit& editable_circuit, bool random_modes,
                    int max_tries = 100'000) -> void;

struct WiresButtonsParams {
    bool random_modes {false};
    int tries_start {5};
    int tries_end {100};
    grid_t grid_start {5};
    grid_t grid_end {10};
    grid_t max_length {50};
};

auto add_many_wires_and_buttons(Rng& rng, EditableCircuit& editable_circuit,
                                WiresButtonsParams params = {}) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
