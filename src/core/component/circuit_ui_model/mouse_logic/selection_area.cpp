#include "core/component/circuit_ui_model/mouse_logic/selection_area.h"

#include "core/algorithm/sort_pair.h"
#include "core/editable_circuit.h"
#include "core/geometry/scene.h"
#include "core/vocabulary/mouse_event.h"

namespace logicsim {

namespace circuit_ui_model {

namespace {

auto calculate_rect(std::optional<point_fine_t> first_position,
                    point_fine_t position) -> rect_fine_t {
    if (!first_position) {
        return rect_fine_t {position, position};
    }

    // order points
    const auto q0 = *first_position;
    const auto q1 = position;
    const auto [x0, x1] = sorted(q0.x, q1.x);
    const auto [y0, y1] = sorted(q0.y, q1.y);

    // QRect
    const auto q_minimum = point_fine_t {x0, y0};
    const auto q_maximum = point_fine_t {x1, y1};
    return rect_fine_t {q_minimum, q_maximum};
}

}  // namespace

auto SelectionAreaLogic::mouse_press(EditableCircuit& editable_circuit,
                                     point_fine_t position,
                                     KeyboardModifiers modifiers) -> void {
    const auto function = [modifiers] {
        if (modifiers == KeyboardModifier::Alt) {
            return SelectionFunction::substract;
        }
        return SelectionFunction::add;
    }();

    if (!modifiers) {
        editable_circuit.clear_visible_selection();
    }

    editable_circuit.add_visible_selection_rect(function,
                                                rect_fine_t {position, position});
    first_position_ = position;
    keep_last_selection_ = false;
}

auto SelectionAreaLogic::mouse_move(EditableCircuit& editable_circuit,
                                    point_fine_t position) -> void {
    update_mouse_position(editable_circuit, position);
}

auto SelectionAreaLogic::mouse_release(EditableCircuit& editable_circuit,
                                       point_fine_t position) -> void {
    update_mouse_position(editable_circuit, position);
    keep_last_selection_ = true;
}

auto SelectionAreaLogic::finalize(EditableCircuit& editable_circuit) -> void {
    if (!keep_last_selection_) {
        editable_circuit.try_pop_last_visible_selection_rect();
    }
    editable_circuit.apply_all_visible_selection_operations();

    // reset
    first_position_.reset();
    keep_last_selection_ = false;
    // rubber_band.hide(); // TODO: hide

    editable_circuit.finish_undo_group();
}

auto SelectionAreaLogic::update_mouse_position(EditableCircuit& editable_circuit,
                                               point_fine_t position) -> void {
    const auto rect = calculate_rect(first_position_, position);

    // TODO: add
    // rubber_band.setGeometry(q_rect);
    // rubber_band.show();

    editable_circuit.try_update_last_visible_selection_rect(rect);
}

}  // namespace circuit_ui_model

}  // namespace logicsim
