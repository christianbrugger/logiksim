#ifndef LOGIKSIM_EDITABLE_CIRCUIT_H
#define LOGIKSIM_EDITABLE_CIRCUIT_H

#include "circuit_layout.h"
#include "schematic.h"

namespace logicsim {

class EditableCircuit {
   public:
    [[nodiscard]] EditableCircuit(Schematic &&schematic, CircuitLayout &&layout);

    [[nodiscard]] auto schematic() const noexcept -> const Schematic &;
    [[nodiscard]] auto layout() const noexcept -> const CircuitLayout &;

    auto add_standard_element(ElementType type, std::size_t input_count, point_t position,
                              DisplayOrientation orientation
                              = DisplayOrientation::default_right) -> void;

    auto add_wire(LineTree &&line_tree) -> void;

   private:
    Schematic schematic_;
    CircuitLayout layout_;
};

}  // namespace logicsim

#endif
