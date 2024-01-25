#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_INSERT_WIRE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_INSERT_WIRE_H

#include "component/circuit_widget/mouse_logic/editing_logic_interface.h"
#include "vocabulary/selection_id.h"
#include "vocabulary/point.h"
#include "vocabulary/line_insertion_type.h"

#include <optional>

namespace logicsim {

class EditableCircuit;

namespace circuit_widget {

class InsertWireLogic : public EditingLogicInterface {
   public:
    auto mouse_press(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;
    auto mouse_move(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;
    auto mouse_release(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void override;

   private:
    selection_id_t temp_wire_ {null_selection_id};

    std::optional<point_t> first_position_ {};
    std::optional<LineInsertionType> direction_ {};
};

}  // namespace circuit_widget

}

#endif
