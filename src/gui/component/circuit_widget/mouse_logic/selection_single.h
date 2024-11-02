#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_SINGLE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_SELECTION_SINGLE_H

#include "gui/component/circuit_widget/mouse_logic/editing_logic_concept.h"

namespace logicsim {

struct point_fine_t;
class EditableCircuit;

namespace circuit_widget {

class SelectionSingleLogic {
   public:
    static auto mouse_press(EditableCircuit& editable_circuit, point_fine_t point,
                            bool double_click) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;
};

static_assert(has_mouse_logic_finalize<SelectionSingleLogic>);

}  // namespace circuit_widget

}  // namespace logicsim

#endif
