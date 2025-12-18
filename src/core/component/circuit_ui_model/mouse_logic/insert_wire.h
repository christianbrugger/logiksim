#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_INSERT_WIRE_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_INSERT_WIRE_H

#include "core/component/circuit_ui_model/mouse_logic/editing_logic_concept.h"
#include "core/vocabulary/line_insertion_type.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/selection_id.h"

#include <optional>

namespace logicsim {

class EditableCircuit;

namespace circuit_ui_model {

class InsertWireLogic {
   public:
    auto mouse_press(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;
    auto mouse_move(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;
    auto mouse_release(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;

   private:
    selection_id_t temp_wire_ {null_selection_id};

    std::optional<point_t> first_position_ {};
    std::optional<point_t> second_position_last_ {};
    std::optional<LineInsertionType> direction_ {};
};

static_assert(has_mouse_logic_finalize<InsertWireLogic>);

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
