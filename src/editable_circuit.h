#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "circuit_layout.h"
#include "schematic.h"

namespace logicsim {

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(CircuitLayout &&layout, Circuit &&schematic);

    [[nodiscard]] auto layout() const noexcept -> const CircuitLayout &;

   private:
    CircuitLayout layout_;
    Circuit schematic_;
};

}  // namespace logicsim

#endif
