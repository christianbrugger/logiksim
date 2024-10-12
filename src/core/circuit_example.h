#ifndef LOGICSIM_CIRCUIT_EXAMPLE_H
#define LOGICSIM_CIRCUIT_EXAMPLE_H

namespace logicsim {

class EditableCircuit;

// TODO replace number with enum

auto load_circuit_example(EditableCircuit& editable_circuit, int number) -> void;

[[nodiscard]] auto load_example_with_logging(int number) -> EditableCircuit;

}  // namespace logicsim

#endif
