#include "core/circuit_ui_model.h"

#include "core/circuit_example.h"
#include "core/vocabulary/mouse_event.h"

namespace logicsim {

namespace {

// constexpr auto inline simulation_interval = 20ms;

}

namespace circuit_ui_model {

auto Statistics::format() const -> std::string {
    return fmt::format(
        "Statistics{{\n"
        "  simulation_events_per_second = {},\n"
        "  frames_per_second = {},\n"
        "  pixel_scale = {},\n"
        "  image_size = {}x{}px\n"
        "  render_mode = {},\n"
        "}}",
        simulation_events_per_second, frames_per_second, pixel_scale, image_size.w,
        image_size.h, render_mode);
}

}  // namespace circuit_ui_model

template <>
auto format(circuit_ui_model::UserAction action) -> std::string {
    switch (action) {
        using enum circuit_ui_model::UserAction;

        case clear_circuit:
            return "clear_circuit";
        case reload_circuit:
            return "reload_circuit";

        case undo:
            return "undo";
        case redo:
            return "redo";
        case select_all:
            return "select_all";
        case copy_selected:
            return "copy_selected";
        case paste_from_clipboard:
            return "paste_from_clipboard";
        case cut_selected:
            return "cut_selected";
        case delete_selected:
            return "delete_selected";

        case zoom_in:
            return "zoom_in";
        case zoom_out:
            return "zoom_out";
        case reset_view:
            return "reset_view";
    };

    std::terminate();
}

CircuitUiModel::CircuitUiModel() {
    circuit_store_.set_simulation_config(config_.simulation_config);
    circuit_store_.set_circuit_state(config_.circuit_state);
    circuit_renderer_.set_render_config(config_.render_config);
    // editing_logic_manager_.set_circuit_state(circuit_state_,
    //                                          editable_circuit_pointer(circuit_store_));

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUiModel::set_config(const CircuitUiConfig& new_config) -> void {
    Expects(class_invariant_holds());

    if (config_.circuit_state != new_config.circuit_state) {
        // close dialogs
        // if (!is_editing_state(new_config.circuit_state)) {
        //     close_all_setting_dialogs();
        // }

        // finalize editing if needed
        // editing_logic_manager_.set_circuit_state(
        //     new_state, editable_circuit_pointer(circuit_store_));

        // clear visible selection
        if (is_selection_state(config_.circuit_state)) {
            circuit_store_.editable_circuit().clear_visible_selection();
            circuit_store_.editable_circuit().finish_undo_group();
        }

        // circuit store
        circuit_store_.set_circuit_state(new_config.circuit_state);

        // simulation
        // if (is_simulation(new_config.circuit_state)) {
        //     timer_run_simulation_.setInterval(0);
        //     timer_run_simulation_.start();
        // } else {
        //     timer_run_simulation_.stop();
        // }

        // update & notify
        config_.circuit_state = new_config.circuit_state;
        // TODO: require update()
    }

    if (config_.render_config != new_config.render_config) {
        circuit_renderer_.set_render_config(new_config.render_config);

        // if (new_config.do_benchmark) {
        //     timer_benchmark_render_.start();
        // } else {
        //     timer_benchmark_render_.stop();
        // }

        config_.render_config = new_config.render_config;
        // TODO: require update()
    }

    if (config_.simulation_config != new_config.simulation_config) {
        circuit_store_.set_simulation_config(new_config.simulation_config);

        config_.simulation_config = new_config.simulation_config;
        // TODO: require update()
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUiModel::config() const -> CircuitUiConfig {
    Expects(class_invariant_holds());
    return config_;
}

auto CircuitUiModel::do_action(UserAction action) -> void {
    Expects(class_invariant_holds());

    switch (action) {
        using enum UserAction;

        case clear_circuit: {
            set_editable_circuit(EditableCircuit {});
            break;
        }
        case reload_circuit: {
            // finalize_editing();
            // const auto _ = Timer {"Reload Circuit"};
            // auto layout = Layout {circuit_store_.layout()};
            // // clear circuit to free memory
            // do_action(UserAction::clear_circuit);
            // set_editable_circuit(EditableCircuit {std::move(layout)});
            break;
        }

        case undo: {
            // this->undo();
            break;
        }
        case redo: {
            // this->redo();
            break;
        }
        case select_all: {
            // this->select_all();
            break;
        }
        case copy_selected: {
            // this->copy_selected();
            break;
        }
        case paste_from_clipboard: {
            // this->paste_clipboard();
            break;
        }
        case cut_selected: {
            // this->copy_selected();
            // this->delete_selected();
            break;
        }
        case delete_selected: {
            // this->delete_selected();
            break;
        }

        case zoom_in: {
            // this->zoom(+1);
            break;
        }
        case zoom_out: {
            // this->zoom(-1);
            break;
        }
        case reset_view: {
            // circuit_renderer_.set_view_point(ViewConfig {}.view_point());
            // update();
            break;
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUiModel::load_circuit_example(int number) -> void {
    Expects(class_invariant_holds());

    const auto default_view_point = ViewConfig {}.view_point();
    const auto default_simulation_config = SimulationConfig {};

    // clear circuit to free memory
    do_action(UserAction::clear_circuit);
    set_editable_circuit(load_example_with_logging(number), default_view_point,
                         default_simulation_config);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUiModel::render(BLImage& bl_image,
                            device_pixel_ratio_t device_pixel_ratio) -> void {
    Expects(class_invariant_holds());

    // TODO use more device_pixel_ratio_t
    circuit_renderer_.set_device_pixel_ratio(double {device_pixel_ratio});

    if (std::holds_alternative<NonInteractiveState>(config_.circuit_state)) {
        circuit_renderer_.render_layout(bl_image, circuit_store_.layout());
    }

    else if (std::holds_alternative<EditingState>(config_.circuit_state)) {
        // const auto show_size_handles =
        // !editing_logic_manager_.is_area_selection_active();
        const auto show_size_handles = false;
        circuit_renderer_.render_editable_circuit(
            bl_image, circuit_store_.editable_circuit(), show_size_handles);
    }

    else if (std::holds_alternative<SimulationState>(config_.circuit_state)) {
        circuit_renderer_.render_simulation(
            bl_image, circuit_store_.interactive_simulation().spatial_simulation());
    }

    else {
        std::terminate();
    }

    // simulation_image_update_pending_ = false;
    // update_history_status();

    Ensures(class_invariant_holds());
}

auto CircuitUiModel::mouse_press(const MousePressEvent& event) -> void {
    print(event);
    if (event.button == MouseButton::Middle) {
        mouse_drag_logic_.mouse_press(event.position);
    }
}

auto CircuitUiModel::mouse_move(const MouseMoveEvent& event) -> void {
    // print(event);
    if (event.buttons.is_set(MouseButton::Middle)) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_move(
                                   event.position, circuit_renderer_.view_config()));
    }
}

auto CircuitUiModel::mouse_release(const MouseReleaseEvent& event) -> void {
    print(event);
    if (event.button == MouseButton::Middle) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_release(
                                   event.position, circuit_renderer_.view_config()));
    }
}

auto CircuitUiModel::set_editable_circuit(
    EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point,
    std::optional<SimulationConfig> simulation_config) -> void {
    Expects(class_invariant_holds());

    // finalize_editing();
    // close_all_setting_dialogs();
    circuit_renderer_.reset();

    // disable simulation
    const auto was_simulation = is_simulation(config_.circuit_state);
    if (was_simulation) {
        set_circuit_state(*this, NonInteractiveState {});
    }

    // set new circuit
    circuit_store_.set_editable_circuit(std::move(editable_circuit));
    if (view_point) {
        circuit_renderer_.set_view_point(view_point.value());
    }
    if (simulation_config) {
        set_simulation_config(*this, simulation_config.value());
    }

    // re-enable simulation
    if (was_simulation) {
        set_circuit_state(*this, SimulationState {});
    }

    // update();
    // TODO update

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUiModel::class_invariant_holds() const -> bool {
    // Configs
    Expects(circuit_renderer_.render_config() == config_.render_config);
    Expects(circuit_store_.simulation_config() == config_.simulation_config);
    Expects(circuit_store_.circuit_state() == config_.circuit_state);
    // Expects(editing_logic_manager_.circuit_state() == config_.circuit_state);
    // Expects(circuit_renderer_.render_config().direct_rendering ==
    //         (this->requested_render_mode() == RenderMode::direct));

    // Setting Dialogs
    // Expects(is_editing_state(circuit_state_) ||
    //         setting_dialog_manager_->open_dialog_count() == 0);

    // if (is_editing_state(config_.circuit_state) &&
    //     !editing_logic_manager_.is_editing_active()) {
    //     // Operation count
    //     Expects(circuit_store_.editable_circuit().visible_selection_operation_count()
    //     ==
    //             0);
    //
    //     // History Group
    //     Expects(!has_ungrouped_undo_entries(circuit_store_.editable_circuit()));
    //
    //     // History Enabled
    //     Expects(is_history_enabled(circuit_store_.editable_circuit()));
    // }

    return true;
}

auto CircuitUiModel::expensive_invariant_holds() const -> bool {
    // insertion state (expensive so only assert)
    // assert(editing_logic_manager_.is_editing_active() ||
    //        all_normal_display_state(circuit_store_.layout()));

    // editable circuit (expensive so only assert)
    assert(!is_editing_state(config_.circuit_state) ||
           is_valid(circuit_store_.editable_circuit()));

    return true;
}

//
// Free Functions
//

auto set_circuit_state(CircuitUiModel& model, CircuitWidgetState value) -> void {
    auto config = model.config();
    config.circuit_state = value;
    model.set_config(config);
}

auto set_render_config(CircuitUiModel& model, WidgetRenderConfig value) -> void {
    auto config = model.config();
    config.render_config = value;
    model.set_config(config);
}

auto set_simulation_config(CircuitUiModel& model, SimulationConfig value) -> void {
    auto config = model.config();
    config.simulation_config = value;
    model.set_config(config);
}
}  // namespace logicsim
