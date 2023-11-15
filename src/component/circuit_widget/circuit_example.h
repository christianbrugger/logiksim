#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_EXAMPLE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_CIRCUIT_EXAMPLE_H

namespace logicsim {

class EditableCircuit;

namespace circuit_widget {

auto load_example_with_logging(EditableCircuit &editable_circuit, int number) -> void;

}

}  // namespace logicsim

#endif
