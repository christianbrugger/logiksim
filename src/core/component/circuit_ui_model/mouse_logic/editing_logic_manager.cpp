#include "core/component/circuit_ui_model/mouse_logic/editing_logic_manager.h"

#include "core/algorithm/overload.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_logic_status.h"
#include "core/default_element_definition.h"
#include "core/editable_circuit.h"
#include "core/geometry/point.h"
#include "core/geometry/scene.h"
#include "core/setting_handle.h"
#include "core/size_handle.h"
#include "core/vocabulary/mouse_event.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_device_fine.h"

#include <gsl/gsl>

namespace logicsim {

namespace circuit_ui_model {

namespace {

auto editing_circuit_valid(const EditableCircuit* editable_circuit,
                           const CircuitWidgetState& circuit_state) {
    return is_editing_state(circuit_state) == (editable_circuit != nullptr);
}

}  // namespace

auto EditingLogicManager::set_circuit_state(CircuitWidgetState new_state,
                                            EditableCircuit* editable_circuit_)
    -> mouse_logic_status_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());
    auto status = mouse_logic_status_t {};

    if (new_state != circuit_state_) {
        status |= finalize_editing(editable_circuit_);

        // update
        circuit_state_ = new_state;
    }

    Ensures(class_invariant_holds());
    return status;
}

auto EditingLogicManager::circuit_state() const -> CircuitWidgetState {
    Expects(class_invariant_holds());

    return circuit_state_;
}

auto EditingLogicManager::finalize_editing(EditableCircuit* editable_circuit_)
    -> mouse_logic_status_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    const auto had_mouse_logic = mouse_logic_.has_value();

    if (mouse_logic_) {
        Expects(editable_circuit_);

        std::visit(
            [&](has_mouse_logic_finalize auto& arg) { arg.finalize(*editable_circuit_); },
            mouse_logic_.value());
        mouse_logic_.reset();
    }

    Ensures(class_invariant_holds());
    Ensures(!mouse_logic_);
    return mouse_logic_status_t {
        .require_repaint = had_mouse_logic,
    };
}

auto EditingLogicManager::confirm_editing(EditableCircuit* editable_circuit_)
    -> mouse_logic_status_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());
    auto status = mouse_logic_status_t {};

    const auto had_mouse_logic = mouse_logic_.has_value();

    if (editable_circuit_ != nullptr && mouse_logic_) {
        bool finished = std::visit(  //
            overload {
                [&](SelectionMoveLogic& arg) {
                    arg.confirm();
                    return arg.is_finished();
                },
                [&](auto&) { return false; },
            },
            *mouse_logic_);

        if (finished) {
            status |= finalize_editing(editable_circuit_);
        }
    }
    // TODO: more fine grained
    status.require_repaint = had_mouse_logic;

    Ensures(class_invariant_holds());
    return status;
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

auto EditingLogicManager::setup_colliding_move(const EditableCircuit& editable_circuit,
                                               std::vector<point_t> cross_points_)
    -> void {
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

auto create_editing_mouse_logic(point_fine_t position, const ViewConfig& view_config,
                                KeyboardModifiers modifiers,
                                const EditableCircuit& editable_circuit,
                                EditingState editing_state)
    -> std::optional<EditingMouseLogic> {
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
                position, editable_circuit.layout(), editable_circuit.visible_selection(),
                view_config)) {
            return HandleResizeLogic {editable_circuit, *size_handle};
        }

        if (const auto setting_handle =
                get_colliding_setting_handle(position, editable_circuit.layout(),
                                             editable_circuit.visible_selection())) {
            return HandleSettingLogic {*setting_handle};
        }

        if (editable_circuit.has_element(position)) {
            if (!modifiers) {
                return SelectionMoveLogic {editable_circuit};
            }
            return SelectionSingleLogic {};
        }
        return SelectionAreaLogic {};
    }

    return std::nullopt;
}

}  // namespace

auto EditingLogicManager::mouse_press(point_fine_t position_fine,
                                      const ViewConfig& view_config,
                                      KeyboardModifiers modifiers, bool double_click,
                                      EditableCircuit* editable_circuit_)
    -> mouse_logic_status_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    if (editable_circuit_ != nullptr && !mouse_logic_) {
        mouse_logic_ = create_editing_mouse_logic(position_fine, view_config, modifiers,
                                                  *editable_circuit_,
                                                  std::get<EditingState>(circuit_state_));
    }

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto position = to_grid(position_fine);

        std::visit(
            overload {[&](InsertLogicItemLogic& arg) {
                          arg.mouse_press(editable_circuit, position);
                      },
                      [&](InsertWireLogic& arg) {
                          arg.mouse_press(editable_circuit, position);
                      },
                      [&](InsertDecorationLogic& arg) {
                          arg.mouse_press(editable_circuit, position);
                      },
                      [&](SelectionAreaLogic& arg) {
                          arg.mouse_press(editable_circuit, position_fine, modifiers);
                      },
                      [&](SelectionSingleLogic& arg) {
                          arg.mouse_press(editable_circuit, position_fine, double_click);
                      },
                      [&](SelectionMoveLogic& arg) {
                          arg.mouse_press(editable_circuit, position_fine, double_click);
                      },
                      [&](HandleResizeLogic& arg) {
                          arg.mouse_press(editable_circuit, position_fine);
                      },
                      [&](HandleSettingLogic& arg) {
                          arg.mouse_press(editable_circuit, position_fine);
                      }},
            mouse_logic_.value());
    }

    Ensures(class_invariant_holds());
    return mouse_logic_status_t {
        .require_repaint = mouse_logic_.has_value(),
    };
}

auto EditingLogicManager::mouse_move(point_fine_t position_fine,
                                     EditableCircuit* editable_circuit_)
    -> mouse_logic_status_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto position = to_grid(position_fine);

        std::visit(overload {[&](InsertLogicItemLogic& arg) {
                                 arg.mouse_move(editable_circuit, position);
                             },
                             [&](InsertWireLogic& arg) {
                                 arg.mouse_move(editable_circuit, position);
                             },
                             [&](InsertDecorationLogic& arg) {
                                 arg.mouse_move(editable_circuit, position);
                             },
                             [&](SelectionAreaLogic& arg) {
                                 arg.mouse_move(editable_circuit, position_fine);
                             },
                             [&](SelectionSingleLogic&) {},
                             [&](SelectionMoveLogic& arg) {
                                 arg.mouse_move(editable_circuit, position_fine);
                             },
                             [&](HandleResizeLogic& arg) {
                                 arg.mouse_move(editable_circuit, position_fine);
                             },
                             [&](HandleSettingLogic&) {}},
                   mouse_logic_.value());
    }

    Ensures(class_invariant_holds());
    return mouse_logic_status_t {
        .require_repaint = mouse_logic_.has_value(),
    };
}

auto EditingLogicManager::mouse_release(point_fine_t position_fine,
                                        EditableCircuit* editable_circuit_,
                                        const OpenSettingDialog& show_setting_dialog)
    -> mouse_logic_status_t {
    Expects(editing_circuit_valid(editable_circuit_, circuit_state_));
    Expects(class_invariant_holds());
    auto status = mouse_logic_status_t {};

    const auto had_mouse_logic = mouse_logic_.has_value();
    // TODO: more fine grained repaint
    status.require_repaint |= had_mouse_logic;

    if (editable_circuit_ != nullptr && mouse_logic_) {
        auto& editable_circuit = *editable_circuit_;
        const auto position = to_grid(position_fine);

        const auto result = std::visit(
            overload {[&](InsertLogicItemLogic& arg) {
                          arg.mouse_release(editable_circuit, position);
                          return mouse_release_status_t {};
                      },
                      [&](InsertWireLogic& arg) {
                          arg.mouse_release(editable_circuit, position);
                          return mouse_release_status_t {};
                      },
                      [&](InsertDecorationLogic& arg) {
                          return arg.mouse_release(editable_circuit, position);
                      },
                      [&](SelectionAreaLogic& arg) {
                          arg.mouse_release(editable_circuit, position_fine);
                          return mouse_release_status_t {};
                      },
                      [&](SelectionSingleLogic&) { return mouse_release_status_t {}; },
                      [&](SelectionMoveLogic& arg) {
                          arg.mouse_release(editable_circuit, position_fine);
                          return mouse_release_status_t {.finished = arg.is_finished()};
                      },
                      [&](HandleResizeLogic& arg) {
                          arg.mouse_release(editable_circuit, position_fine);
                          return mouse_release_status_t {};
                      },
                      [&](HandleSettingLogic& arg) {
                          arg.mouse_release(editable_circuit, position_fine,
                                            show_setting_dialog);
                          return mouse_release_status_t {};
                      }},
            mouse_logic_.value());

        if (result.finished) {
            status |= finalize_editing(editable_circuit_);
        }
        status |= result.mouse_logic_status;
    }

    Ensures(class_invariant_holds());
    return status;
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

    // rubber band // TODO:
    // Expects(rubber_band_.isHidden() ||
    //         (mouse_logic_ &&
    //         std::holds_alternative<SelectionAreaLogic>(*mouse_logic_)));

    return true;
}

}  // namespace circuit_ui_model

}  // namespace logicsim
