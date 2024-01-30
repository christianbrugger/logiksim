#include "component/circuit_widget/mouse_logic/editing_logic_manager.h"

#include "algorithm/overload.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
#include "geometry/scene.h"
#include "logging.h"
#include "setting_handle.h"
#include "size_handle.h"
#include "vocabulary/point.h"

#include <gsl/gsl>

namespace logicsim {

namespace circuit_widget {

namespace {

auto mouse_logic_valid(const std::optional<EditingMouseLogic>& mouse_logic,
                       const CircuitWidgetState& circuit_state) {
    // return !mouse_logic.has_value() || is_editing_state(circuit_state);
    if (!mouse_logic) {
        return true;
    }

    if (!is_editing_state(circuit_state)) {
        return false;
    }
    if (is_insert_logic_item_logic(*mouse_logic) !=
        is_insert_logic_item_state(circuit_state)) {
        return false;
    }
    if (is_insert_wire_logic(*mouse_logic) != is_insert_wire_state(circuit_state)) {
        return false;
    }
    if (is_selecting_logic(*mouse_logic) != is_selection_state(circuit_state)) {
        return false;
    }

    return true;
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
    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
}

auto EditingLogicManager::set_circuit_state(CircuitWidgetState new_state,
                                            EditableCircuit* editable_circuit_) -> void {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (new_state != circuit_state_) {
        finalize_editing(editable_circuit_);

        // update
        circuit_state_ = new_state;
    }

    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
}

auto EditingLogicManager::circuit_state() const -> CircuitWidgetState {
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    return circuit_state_;
}

auto EditingLogicManager::finalize_editing(EditableCircuit* editable_circuit_)
    -> ManagerResult {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    const auto res = mouse_logic_ ? ManagerResult::require_update : ManagerResult::done;

    if (editable_circuit_ && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;

        std::visit(
            overload([&](EditingLogicInterface& arg) { arg.finalize(editable_circuit); },
                     [&](SelectionAreaLogic& arg) {
                         arg.finalize(editable_circuit, rubber_band_);
                     }),
            mouse_logic_.value());
        mouse_logic_.reset();
    }

    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    Ensures(!mouse_logic_);
    return res;
}

auto EditingLogicManager::confirm_editing(EditableCircuit* editable_circuit_)
    -> ManagerResult {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    const auto res = mouse_logic_ ? ManagerResult::require_update : ManagerResult::done;

    if (editable_circuit_ && mouse_logic_) {
        bool finished = std::visit(overload {
                                       [&](SelectionMoveLogic& arg) {
                                           arg.confirm();
                                           return arg.is_finished();
                                       },
                                       [&](auto&) { return false; },
                                   },
                                   *mouse_logic_);

        if (finished) {
            finalize_editing(editable_circuit_);
        }
    }

    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return res;
}

auto EditingLogicManager::is_editing_active() const -> bool {
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    return mouse_logic_.has_value();
}

auto EditingLogicManager::is_area_selection_active() const -> bool {
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    return mouse_logic_.has_value() &&
           std::holds_alternative<SelectionAreaLogic>(*mouse_logic_);
}

auto EditingLogicManager::setup_colliding_move(const EditableCircuit& editable_circuit,
                                               std::vector<point_t> cross_points__)
    -> void {
    Expects(is_selection_state(circuit_state_));
    Expects(!mouse_logic_);

    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    mouse_logic_.emplace(SelectionMoveLogic {
        editable_circuit, SelectionMoveLogic::Args {
                              .has_colliding = true,
                              .delete_on_cancel = true,
                              .cross_points = std::move(cross_points__),
                          }});
}

namespace {

auto create_editing_mouse_logic(QPointF position, const ViewConfig& view_config,
                                Qt::KeyboardModifiers modifiers,
                                const EditableCircuit& editable_circuit, QWidget& parent,
                                EditingState editing_state)
    -> std::optional<EditingMouseLogic> {
    const auto grid_fine_position = to_grid_fine(position, view_config);

    // insert logic items
    if (is_insert_logic_item_state(editing_state)) {
        return InsertLogicItemLogic {
            to_logic_item_definition(editing_state.default_mouse_action),
        };
    }

    // insert wires
    if (is_insert_wire_state(editing_state)) {
        return InsertWireLogic {};
    }

    // selection
    if (is_selection_state(editing_state)) {
        if (const auto size_handle = get_colliding_size_handle(
                grid_fine_position, editable_circuit.layout(),
                editable_circuit.visible_selection(), view_config)) {
            return HandleResizeLogic {editable_circuit, *size_handle};
        }

        if (const auto setting_handle = get_colliding_setting_handle(
                grid_fine_position, editable_circuit.layout(),
                editable_circuit.visible_selection())) {
            return HandleSettingLogic {*setting_handle};
        }

        if (editable_circuit.caches().selection_index().has_element(grid_fine_position)) {
            if (modifiers == Qt::NoModifier) {
                return SelectionMoveLogic {editable_circuit};
            }
            return SelectionSingleLogic {};
        }
        return SelectionAreaLogic {};
    }

    return std::nullopt;
}

}  // namespace

auto EditingLogicManager::mouse_press(QPointF position, const ViewConfig& view_config,
                                      Qt::KeyboardModifiers modifiers, bool double_click,
                                      EditableCircuit* editable_circuit_, QWidget& parent)
    -> ManagerResult {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (editable_circuit_ && !mouse_logic_) {
        mouse_logic_ = create_editing_mouse_logic(position, view_config, modifiers,
                                                  *editable_circuit_, parent,
                                                  std::get<EditingState>(circuit_state_));
    }

    if (editable_circuit_ && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto grid_position = to_grid(position, view_config);
        const auto grid_fine_position = to_grid_fine(position, view_config);

        std::visit(
            overload {
                [&](InsertLogicItemLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_position);
                },
                [&](InsertWireLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_position);
                },
                [&](SelectionAreaLogic& arg) {
                    arg.mouse_press(editable_circuit, position, view_config, modifiers);
                },
                [&](SelectionSingleLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_fine_position, double_click);
                },
                [&](SelectionMoveLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_fine_position, double_click);
                },
                [&](HandleResizeLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_fine_position);
                },
                [&](HandleSettingLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_fine_position);
                }},
            mouse_logic_.value());
    }

    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return mouse_logic_ ? ManagerResult::require_update : ManagerResult::done;
}

auto EditingLogicManager::mouse_move(QPointF position, const ViewConfig& view_config,
                                     EditableCircuit* editable_circuit_)
    -> ManagerResult {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    if (editable_circuit_ && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto grid_position = to_grid(position, view_config);
        const auto grid_fine_position = to_grid_fine(position, view_config);

        std::visit(overload {[&](InsertLogicItemLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_position);
                             },
                             [&](InsertWireLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_position);
                             },
                             [&](SelectionAreaLogic& arg) {
                                 arg.mouse_move(editable_circuit, position, view_config,
                                                rubber_band_);
                             },
                             [&](SelectionSingleLogic&) {},
                             [&](SelectionMoveLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_fine_position);
                             },
                             [&](HandleResizeLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_fine_position);
                             },
                             [&](HandleSettingLogic&) {}},
                   mouse_logic_.value());
    }

    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return mouse_logic_ ? ManagerResult::require_update : ManagerResult::done;
}

auto EditingLogicManager::mouse_release(QPointF position, const ViewConfig& view_config,
                                        EditableCircuit* editable_circuit_,
                                        const OpenSettingDialog& show_setting_dialog)
    -> ManagerResult {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(mouse_logic_valid(mouse_logic_, circuit_state_));
    Expects(rubber_band_valid(rubber_band_, circuit_state_));

    const auto res = mouse_logic_ ? ManagerResult::require_update : ManagerResult::done;

    if (editable_circuit_ && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto grid_position = to_grid(position, view_config);
        const auto grid_fine_position = to_grid_fine(position, view_config);

        const auto finished = std::visit(
            overload {[&](InsertLogicItemLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_position);
                          return true;
                      },
                      [&](InsertWireLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_position);
                          return true;
                      },
                      [&](SelectionAreaLogic& arg) {
                          arg.mouse_release(editable_circuit, position, view_config,
                                            rubber_band_);
                          return true;
                      },
                      [&](SelectionSingleLogic&) { return true; },
                      [&](SelectionMoveLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_fine_position);
                          return arg.is_finished();
                      },
                      [&](HandleResizeLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_fine_position);
                          return true;
                      },
                      [&](HandleSettingLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_fine_position,
                                            show_setting_dialog);
                          return true;
                      }},
            mouse_logic_.value());

        if (finished) {
            finalize_editing(editable_circuit_);
        }
    }

    Ensures(mouse_logic_valid(mouse_logic_, circuit_state_));
    Ensures(rubber_band_valid(rubber_band_, circuit_state_));
    return res;
}

}  // namespace circuit_widget

}  // namespace logicsim
