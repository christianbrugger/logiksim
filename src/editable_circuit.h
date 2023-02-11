#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "circuit_layout.h"
#include "schematic.h"

namespace logicsim {

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(CircuitLayout &&layout, Schematic &&schematic);

    [[nodiscard]] auto layout() const noexcept -> const CircuitLayout &;

   private:
    CircuitLayout layout_;
    Schematic schematic_;
};

}  // namespace logicsim

#endif
