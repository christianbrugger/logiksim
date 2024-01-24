#include "component/circuit_widget/mouse_logic/editing_logic_manager.h"

#include "algorithm/overload.h"
#include "default_element_definition.h"
#include "geometry/scene.h"
#include "logging.h"
#include "vocabulary/point.h"

#include <gsl/gsl>

namespace logicsim {

namespace circuit_widget {

namespace {

auto empty_or_in_editing(const std::optional<EditingMouseLogic>& mouse_logic,
                         const CircuitWidgetState& circuit_state) {
    return !mouse_logic.has_value() || is_editing_state(circuit_state);
}

}  // namespace

auto EditingLogicManager::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));

    if (new_state == circuit_state_) {
        return;
    }

    // update
    circuit_state_ = new_state;

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
}

auto logicsim::circuit_widget::EditingLogicManager::circuit_state() const
    -> CircuitWidgetState {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    return circuit_state_;
}

namespace {

auto get_editing_mouse_logic(QPointF position, const ViewConfig& view_config,
                             EditingState editing_state)
    -> std::optional<circuit_widget::EditingMouseLogic> {
    // insert logic items
    if (is_insert_logic_item_state(editing_state)) {
        return circuit_widget::InsertLogicItemLogic {
            to_logic_item_definition(editing_state.default_mouse_action),
        };
    }

    // insert wires
    if (is_insert_wire_state(editing_state)) {
        print("TODO insert wire mouse logic");
        return std::nullopt;
    }

    // selection
    if (is_selection_state(editing_state)) {
        print("TODO selection mouse logic");
        return std::nullopt;
    }

    return std::nullopt;
}

}  // namespace

auto logicsim::circuit_widget::EditingLogicManager::mouse_press(
    QPointF position, const ViewConfig& view_config, Qt::MouseButton button,
    EditableCircuit& editable_circuit) -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));

    if (button == Qt::LeftButton) {
        if (!mouse_logic_ && is_editing_state(circuit_state_)) {
            mouse_logic_ = get_editing_mouse_logic(
                position, view_config, std::get<EditingState>(circuit_state_));
        }

        if (mouse_logic_) {
            const auto grid_position = to_grid(position, view_config);

            std::visit(overload {[&](InsertLogicItemLogic& arg) {
                           arg.mouse_press(editable_circuit, grid_position);
                       }},
                       mouse_logic_.value());

            Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
            return ManagerResult::require_update;
        }
    }

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    return ManagerResult::done;
}

auto logicsim::circuit_widget::EditingLogicManager::mouse_move(
    QPointF position, const ViewConfig& view_config, EditableCircuit& editable_circuit)
    -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));

    if (mouse_logic_) {
        const auto grid_position = to_grid(position, view_config);

        std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                       arg.mouse_move(editable_circuit, grid_position);
                   }},
                   mouse_logic_.value());

        Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
        return ManagerResult::require_update;
    }

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    return ManagerResult::done;
}

auto logicsim::circuit_widget::EditingLogicManager::mouse_release(
    QPointF position, const ViewConfig& view_config, EditableCircuit& editable_circuit)
    -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));

    if (mouse_logic_) {
        const auto grid_position = to_grid(position, view_config);

        std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                       arg.mouse_release(editable_circuit, grid_position);
                   }},
                   mouse_logic_.value());

        std::visit(
            [&](circuit_widget::EditingLogicInterface& arg) {
                arg.finalize(editable_circuit);
            },
            mouse_logic_.value());
        mouse_logic_.reset();

        Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
        return ManagerResult::require_update;
    }

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    return ManagerResult::done;
}

}  // namespace circuit_widget

}  // namespace logicsim
