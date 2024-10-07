#ifndef LOGICSIM_GUI_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_INSERT_DECORATION_H
#define LOGICSIM_GUI_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_INSERT_DECORATION_H

#include "component/circuit_widget/mouse_logic/editing_logic_concept.h"
#include "vocabulary/decoration_definition.h"
#include "vocabulary/selection_id.h"

#include <optional>

namespace logicsim {

struct point_t;
class EditableCircuit;

namespace circuit_widget {

class InsertDecorationLogic {
   public:
    explicit InsertDecorationLogic(DecorationDefinition element_definition_);

    auto mouse_press(EditableCircuit& editable_circuit,
                     std::optional<point_t> position) -> void;
    auto mouse_move(EditableCircuit& editable_circuit,
                    std::optional<point_t> position) -> void;
    auto mouse_release(EditableCircuit& editable_circuit,
                       std::optional<point_t> position) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;

   private:
    DecorationDefinition element_definition_;
    selection_id_t temp_element_ {null_selection_id};
};

static_assert(has_mouse_logic_finalize<InsertDecorationLogic>);

}  // namespace circuit_widget

}  // namespace logicsim

#endif
