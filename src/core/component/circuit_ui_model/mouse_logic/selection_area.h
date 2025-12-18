#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_SELECTION_AREA_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_SELECTION_AREA_H

#include "core/vocabulary/point_fine.h"

#include <optional>

namespace logicsim {

class KeyboardModifiers;
struct point_t;
struct point_fine_t;
struct ViewConfig;
class EditableCircuit;

namespace circuit_ui_model {

class SelectionAreaLogic {
   public:
    auto mouse_press(EditableCircuit& editable_circuit, point_fine_t position,
                     KeyboardModifiers modifiers) -> void;
    auto mouse_move(EditableCircuit& editable_circuit, point_fine_t position) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, point_fine_t position) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;

   private:
    auto update_mouse_position(EditableCircuit& editable_circuit, point_fine_t position)
        -> void;

   private:
    std::optional<point_fine_t> first_position_ {};
    bool keep_last_selection_ {false};
};

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
