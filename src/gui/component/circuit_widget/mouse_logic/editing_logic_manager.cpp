#include "gui/component/circuit_widget/mouse_logic/editing_logic_manager.h"

#include "gui/component/circuit_widget/mouse_logic/editing_logic_result.h"
#include "gui/qt/point_conversion.h"

#include "core/algorithm/overload.h"
#include "core/default_element_definition.h"
#include "core/editable_circuit.h"
#include "core/geometry/scene.h"
#include "core/setting_handle.h"
#include "core/size_handle.h"
#include "core/vocabulary/point.h"

#include <gsl/gsl>

namespace logicsim {

namespace circuit_widget {

namespace {

auto editing_circuit_valid(const EditableCircuit* editable_circuit,
                           const CircuitWidgetState& circuit_state) {
    return is_editing_state(circuit_state) == (editable_circuit != nullptr);
}

}  // namespace

EditingLogicManager::EditingLogicManager(QWidget* parent)
    : rubber_band_ {QRubberBand::Rectangle, parent} {
    Ensures(class_invariant_holds());
}

auto EditingLogicManager::set_circuit_state(CircuitWidgetState new_state,
                                            EditableCircuit* editable_circuit_) -> void {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    if (new_state != circuit_state_) {
        finalize_editing(editable_circuit_);

        // update
        circuit_state_ = new_state;
    }

    Ensures(class_invariant_holds());
}

auto EditingLogicManager::circuit_state() const -> CircuitWidgetState {
    Expects(class_invariant_holds());

    return circuit_state_;
}

auto EditingLogicManager::finalize_editing(EditableCircuit* editable_circuit_)
    -> editing_logic_result_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    const auto had_mouse_logic = mouse_logic_.has_value();

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;

        std::visit(overload(
                       [&](has_mouse_logic_finalize auto& arg) {
                           arg.finalize(editable_circuit);
                       },
                       [&](SelectionAreaLogic& arg) {
                           arg.finalize(editable_circuit, rubber_band_);
                       }),
                   mouse_logic_.value());
        mouse_logic_.reset();
    }

    Ensures(class_invariant_holds());
    Ensures(!mouse_logic_);
    return editing_logic_result_t {
        .require_update = had_mouse_logic,
    };
}

auto EditingLogicManager::confirm_editing(EditableCircuit* editable_circuit_)
    -> editing_logic_result_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    const auto had_mouse_logic = mouse_logic_.has_value();

    if (editable_circuit_ != nullptr && mouse_logic_) {
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

    Ensures(class_invariant_holds());
    return editing_logic_result_t {
        .require_update = had_mouse_logic,
    };
}

auto EditingLogicManager::is_editing_active() const -> bool {
    Expects(class_invariant_holds());

    return mouse_logic_.has_value();
}

auto EditingLogicManager::is_area_selection_active() const -> bool {
    Expects(class_invariant_holds());

    return mouse_logic_.has_value() &&
           std::holds_alternative<SelectionAreaLogic>(*mouse_logic_);
}

auto EditingLogicManager::setup_colliding_move(
    const EditableCircuit& editable_circuit, std::vector<point_t> cross_points_) -> void {
    Expects(class_invariant_holds());

    Expects(is_selection_state(circuit_state_));
    Expects(!mouse_logic_);

    mouse_logic_.emplace(SelectionMoveLogic {
        editable_circuit,
        SelectionMoveLogic::Args {
            .has_colliding = true,
            .delete_on_cancel = true,
            .cross_points = std::move(cross_points_),
        },
    });

    Ensures(class_invariant_holds());
}

namespace {

auto create_editing_mouse_logic(
    QPointF position, const ViewConfig& view_config, Qt::KeyboardModifiers modifiers,
    const EditableCircuit& editable_circuit,
    EditingState editing_state) -> std::optional<EditingMouseLogic> {
    const auto grid_fine_position = to_grid_fine(to(position), view_config);

    // insert logic items
    if (is_insert_logicitem_state(editing_state)) {
        return InsertLogicItemLogic {
            to_logicitem_definition(editing_state.default_mouse_action),
        };
    }

    // insert wires
    if (is_insert_wire_state(editing_state)) {
        return InsertWireLogic {};
    }

    // insert decorations
    if (is_insert_decoration_state(editing_state)) {
        return InsertDecorationLogic {
            to_decoration_definition(editing_state.default_mouse_action),
        };
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

        if (editable_circuit.has_element(grid_fine_position)) {
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

auto EditingLogicManager::mouse_press(
    QPointF position, const ViewConfig& view_config, Qt::KeyboardModifiers modifiers,
    bool double_click, EditableCircuit* editable_circuit_) -> editing_logic_result_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    if (editable_circuit_ != nullptr && !mouse_logic_) {
        mouse_logic_ = create_editing_mouse_logic(position, view_config, modifiers,
                                                  *editable_circuit_,
                                                  std::get<EditingState>(circuit_state_));
    }

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto grid_position = to_grid(to(position), view_config);
        const auto grid_fine_position = to_grid_fine(to(position), view_config);

        std::visit(
            overload {
                [&](InsertLogicItemLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_position);
                },
                [&](InsertWireLogic& arg) {
                    arg.mouse_press(editable_circuit, grid_position);
                },
                [&](InsertDecorationLogic& arg) {
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

    Ensures(class_invariant_holds());
    return editing_logic_result_t {
        .require_update = mouse_logic_.has_value(),
    };
}

auto EditingLogicManager::mouse_move(QPointF position, const ViewConfig& view_config,
                                     EditableCircuit* editable_circuit_)
    -> editing_logic_result_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto grid_position = to_grid(to(position), view_config);
        const auto grid_fine_position = to_grid_fine(to(position), view_config);

        std::visit(overload {[&](InsertLogicItemLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_position);
                             },
                             [&](InsertWireLogic& arg) {
                                 arg.mouse_move(editable_circuit, grid_position);
                             },
                             [&](InsertDecorationLogic& arg) {
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

    Ensures(class_invariant_holds());
    return editing_logic_result_t {
        .require_update = mouse_logic_.has_value(),
    };
}

auto EditingLogicManager::mouse_release(
    QPointF position, const ViewConfig& view_config, EditableCircuit* editable_circuit_,
    const OpenSettingDialog& show_setting_dialog) -> editing_logic_result_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    const auto had_mouse_logic = mouse_logic_.has_value();

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto grid_position = to_grid(to(position), view_config);
        const auto grid_fine_position = to_grid_fine(to(position), view_config);

        const auto finished = std::visit(
            overload {[&](InsertLogicItemLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_position);
                          return true;
                      },
                      [&](InsertWireLogic& arg) {
                          arg.mouse_release(editable_circuit, grid_position);
                          return true;
                      },
                      [&](InsertDecorationLogic& arg) {
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

    Ensures(class_invariant_holds());
    return editing_logic_result_t {
        .require_update = had_mouse_logic,
    };
}

auto EditingLogicManager::class_invariant_holds() const -> bool {
    // mouse logic
    Expects(!mouse_logic_ || is_editing_state(circuit_state_));
    Expects(!mouse_logic_ || is_insert_logicitem_logic(*mouse_logic_) ==
                                 is_insert_logicitem_state(circuit_state_));
    Expects(!mouse_logic_ ||
            is_insert_wire_logic(*mouse_logic_) == is_insert_wire_state(circuit_state_));
    Expects(!mouse_logic_ || is_insert_decoration_logic(*mouse_logic_) ==
                                 is_insert_decoration_state(circuit_state_));
    Expects(!mouse_logic_ ||
            is_selecting_logic(*mouse_logic_) == is_selection_state(circuit_state_));

    // rubber band
    Expects(rubber_band_.isHidden() ||
            (mouse_logic_ && std::holds_alternative<SelectionAreaLogic>(*mouse_logic_)));

    return true;
}

}  // namespace circuit_widget

}  // namespace logicsim
