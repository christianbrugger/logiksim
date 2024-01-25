#include "component/circuit_widget/mouse_logic/editing_logic_manager.h"

#include "algorithm/overload.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
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

auto editing_circuit_valid(const EditableCircuit* editable_circuit,
                           const CircuitWidgetState& circuit_state) {
    return is_editing_state(circuit_state) == (editable_circuit != nullptr);
}

auto rubber_band_valid(const QRubberBand& rubber_band,
                       const CircuitWidgetState& circuit_state) {
    return rubber_band.isHidden() || is_selection_state(circuit_state);
}

}  // namespace

EditingLogicManager::EditingLogicManager(QWidget* parent)
    : rubber_band_ {QRubberBand::Rectangle, parent} {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));
}

auto EditingLogicManager::set_circuit_state(CircuitWidgetState new_state,
                                            EditableCircuit* editable_circuit_) -> void {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (new_state == circuit_state_) {
        return;
    }

    if (!is_editing_state(new_state)) {
        finalize_editing(editable_circuit_);
    }

    // update
    circuit_state_ = new_state;

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
}

auto EditingLogicManager::circuit_state() const -> CircuitWidgetState {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    return circuit_state_;
}

auto EditingLogicManager::finalize_editing(EditableCircuit* editable_circuit_)
    -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (!editable_circuit_) {
        Ensures(!mouse_logic_);
        return ManagerResult::done;
    }
    auto& editable_circuit = *editable_circuit_;

    if (mouse_logic_) {
        std::visit(overload(
                       [&](circuit_widget::EditingLogicInterface& arg) {
                           arg.finalize(editable_circuit);
                       },
                       [&](circuit_widget::SelectionAreaLogic& arg) {
                           arg.finalize(editable_circuit, rubber_band_);
                       }),
                   mouse_logic_.value());
        mouse_logic_.reset();

        Ensures(!mouse_logic_);
        Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
        Ensures(rubber_band_valid(rubber_band_, circuit_state_));
        return ManagerResult::require_update;
    }

    Ensures(!mouse_logic_);
    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return ManagerResult::done;
}

auto EditingLogicManager::is_editing_active() const -> bool {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    return mouse_logic_.has_value();
}

namespace {

auto create_editing_mouse_logic(QPointF position, const ViewConfig& view_config,
                                Qt::KeyboardModifiers modifiers,
                                EditableCircuit& editable_circuit, QWidget& parent,
                                EditingState editing_state)
    -> std::optional<circuit_widget::EditingMouseLogic> {
    const auto grid_fine_position = to_grid_fine(position, view_config);

    // insert logic items
    if (is_insert_logic_item_state(editing_state)) {
        return circuit_widget::InsertLogicItemLogic {
            to_logic_item_definition(editing_state.default_mouse_action),
        };
    }

    // insert wires
    if (is_insert_wire_state(editing_state)) {
        return circuit_widget::InsertWireLogic {};
    }

    // selection
    if (is_selection_state(editing_state)) {
        if (editable_circuit.caches().selection_index().has_element(grid_fine_position)) {
            if (modifiers == Qt::NoModifier) {
                print("MouseMoveSelectionLogic");
                return std::nullopt;
            }

            return circuit_widget::SelectionSingleLogic {};
        }
        return circuit_widget::SelectionAreaLogic {};
    }

    return std::nullopt;
}

}  // namespace

auto EditingLogicManager::mouse_press(QPointF position, const ViewConfig& view_config,
                                      Qt::KeyboardModifiers modifiers, bool double_click,
                                      EditableCircuit* editable_circuit_, QWidget& parent)
    -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (!editable_circuit_) {
        return ManagerResult::done;
    }
    auto& editable_circuit = *editable_circuit_;

    if (!mouse_logic_) {
        mouse_logic_ =
            create_editing_mouse_logic(position, view_config, modifiers, editable_circuit,
                                       parent, std::get<EditingState>(circuit_state_));
    }

    if (mouse_logic_) {
        const auto grid_position = to_grid(position, view_config);
        const auto grid_fine_position = to_grid_fine(position, view_config);

        std::visit(overload {[&](InsertLogicItemLogic& arg) {
                                 arg.mouse_press(editable_circuit, grid_position);
                             },
                             [&](InsertWireLogic& arg) {
                                 arg.mouse_press(editable_circuit, grid_position);
                             },
                             [&](SelectionAreaLogic& arg) {
                                 arg.mouse_press(editable_circuit, position, view_config,
                                                 modifiers);
                             },
                             [&](SelectionSingleLogic& arg) {
                                 arg.mouse_press(editable_circuit, grid_fine_position,
                                                 double_click);
                             }},
                   mouse_logic_.value());

        Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
        Ensures(rubber_band_valid(rubber_band_, circuit_state_));
        return ManagerResult::require_update;
    }

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return ManagerResult::done;
}

auto EditingLogicManager::mouse_move(QPointF position, const ViewConfig& view_config,
                                     EditableCircuit* editable_circuit_)
    -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (!editable_circuit_) {
        return ManagerResult::done;
    }
    auto& editable_circuit = *editable_circuit_;

    if (mouse_logic_) {
        const auto grid_position = to_grid(position, view_config);
        // const auto grid_fine_position = to_grid_fine(position, view_config);

        std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_position);
                             },
                             [&](circuit_widget::InsertWireLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_position);
                             },
                             [&](circuit_widget::SelectionAreaLogic& arg) {
                                 arg.mouse_move(editable_circuit, position, view_config,
                                                rubber_band_);
                             },
                             [&](circuit_widget::SelectionSingleLogic&) {}},
                   mouse_logic_.value());

        Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
        Ensures(rubber_band_valid(rubber_band_, circuit_state_));
        return ManagerResult::require_update;
    }

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return ManagerResult::done;
}

auto EditingLogicManager::mouse_release(QPointF position, const ViewConfig& view_config,
                                        EditableCircuit* editable_circuit_)
    -> ManagerResult {
    Expects(empty_or_in_editing(mouse_logic_, circuit_state_));
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (!editable_circuit_) {
        return ManagerResult::done;
    }
    auto& editable_circuit = *editable_circuit_;

    if (mouse_logic_) {
        const auto grid_position = to_grid(position, view_config);
        // const auto grid_fine_position = to_grid_fine(position, view_config);

        std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                                 arg.mouse_release(editable_circuit, grid_position);
                             },
                             [&](circuit_widget::InsertWireLogic& arg) {
                                 arg.mouse_release(editable_circuit, grid_position);
                             },
                             [&](circuit_widget::SelectionAreaLogic& arg) {
                                 arg.mouse_release(editable_circuit, position,
                                                   view_config, rubber_band_);
                             },
                             [&](circuit_widget::SelectionSingleLogic&) {}},
                   mouse_logic_.value());

        finalize_editing(editable_circuit_);

        Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
        Ensures(rubber_band_valid(rubber_band_, circuit_state_));
        return ManagerResult::require_update;
    }

    Ensures(empty_or_in_editing(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return ManagerResult::done;
}

}  // namespace circuit_widget

}  // namespace logicsim
