#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_INSERT_LOGIC_ITEM_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_INSERT_LOGIC_ITEM_H

#include "component/circuit_widget/mouse_logic/editing_logic_interface.h"
#include "vocabulary/logicitem_definition.h"
#include "vocabulary/selection_id.h"

#include <optional>

namespace logicsim {

struct point_t;
class EditableCircuit;

namespace circuit_widget {

class InsertLogicItemLogic : public EditingLogicInterface {
   public:
    InsertLogicItemLogic(LogicItemDefinition element_definition);

    auto mouse_press(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;
    auto mouse_move(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;
    auto mouse_release(EditableCircuit& editable_circuit, std::optional<point_t> position)
        -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void override;

   private:
    LogicItemDefinition element_definition_;
    selection_id_t temp_element_ {null_selection_id};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
