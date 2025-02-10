#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_HANDLE_RESIZE_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_MOUSE_LOGIC_HANDLE_RESIZE_H

#include "core/component/circuit_ui_model/mouse_logic/editing_logic_concept.h"
#include "core/size_handle.h"
#include "core/vocabulary/placed_element.h"
#include "core/vocabulary/point_fine.h"

#include <optional>

namespace logicsim {

struct point_fine_t;
class EditableCircuit;

namespace circuit_ui_model {

class HandleResizeLogic {
   public:
    explicit HandleResizeLogic(const EditableCircuit& editable_circuit,
                               size_handle_t size_handle);

    auto mouse_press(EditableCircuit& editable_circuit, point_fine_t position) -> void;
    auto mouse_move(EditableCircuit& editable_circuit, point_fine_t position) -> void;
    auto mouse_release(EditableCircuit& editable_circuit, point_fine_t position) -> void;

    auto finalize(EditableCircuit& editable_circuit) -> void;

   private:
    auto move_handle_to(EditableCircuit& editable_circuit, point_fine_t position) -> void;

   private:
    size_handle_t size_handle_;
    PlacedElement initial_element_;

    std::optional<point_fine_t> first_position_ {};
    std::optional<delta_movement_t> last_delta_ {};
};

static_assert(has_mouse_logic_finalize<HandleResizeLogic>);

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
