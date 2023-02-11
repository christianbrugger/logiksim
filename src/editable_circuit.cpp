#include "editable_circuit.h"

namespace logicsim {

EditableCircuit::EditableCircuit(CircuitLayout&& layout, Schematic&& schematic)
    : layout_ {std::move(layout)}, schematic_ {std::move(schematic)} {}

auto EditableCircuit::layout() const noexcept -> const CircuitLayout& {
    return layout_;
}

}  // namespace logicsim
