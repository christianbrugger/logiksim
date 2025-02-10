#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_SELECTION_SINGLE_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_SELECTION_SINGLE_H

#include "core/component/circuit_ui_model/mouse_logic/editing_logic_concept.h"

namespace logicsim {

struct point_fine_t;
class EditableCircuit;

namespace circuit_ui_model {

class SelectionSingleLogic {
   public:
    static auto mouse_press(EditableCircuit& editable_circuit, point_fine_t point,
                            bool double_click) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;
};

static_assert(has_mouse_logic_finalize<SelectionSingleLogic>);

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
