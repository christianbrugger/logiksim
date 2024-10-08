#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_RESIZE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_MOUSE_LOGIC_HANDLE_RESIZE_H

#include "component/circuit_widget/mouse_logic/editing_logic_concept.h"
#include "size_handle.h"
#include "vocabulary/placed_element.h"
#include "vocabulary/point_fine.h"
#include "vocabulary/selection_id.h"

#include <optional>

namespace logicsim {

struct point_fine_t;
class EditableCircuit;

namespace circuit_widget {

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
    PlacedElement initial_logicitem_;

    std::optional<point_fine_t> first_position_ {};
    std::optional<int> last_delta_ {};
};

static_assert(has_mouse_logic_finalize<HandleResizeLogic>);

}  // namespace circuit_widget

}  // namespace logicsim

#endif
