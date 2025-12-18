#include "core/circuit_ui_model.h"

#include "core/circuit_example.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_logic_status.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_wheel_logic.h"
#include "core/geometry/scene.h"
#include "core/vocabulary/allocation_info.h"
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

CircuitUIModel::CircuitUIModel() {
    circuit_store_.set_simulation_config(config_.simulation);
    circuit_store_.set_circuit_state(config_.state);
    circuit_renderer_.set_render_config(config_.render);
    static_cast<void>(editing_logic_manager_.set_circuit_state(
        config_.state, editable_circuit_pointer(circuit_store_)));

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitUIModel::set_config(const CircuitUIConfig& new_config) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (config_.state != new_config.state) {
        // close dialogs
        if (!is_editing_state(new_config.state)) {
            status |= close_all_setting_dialogs();
        }

        // finalizes editing if needed
        status |= editing_logic_manager_.set_circuit_state(
            new_config.state, editable_circuit_pointer(circuit_store_));

        // clear visible selection
        if (is_selection_state(config_.state)) {
            circuit_store_.editable_circuit().clear_visible_selection();
            circuit_store_.editable_circuit().finish_undo_group();
        }

        // circuit store
        circuit_store_.set_circuit_state(new_config.state);

        // simulation
        // if (is_simulation(new_config.circuit_state)) {
        //     timer_run_simulation_.setInterval(0);
        //     timer_run_simulation_.start();
        // } else {
        //     timer_run_simulation_.stop();
        // }

        // update & notify
        config_.state = new_config.state;
        status.config_changed = true;
        status.require_repaint = true;
    }

    if (config_.render != new_config.render) {
        circuit_renderer_.set_render_config(new_config.render);

        // if (new_config.do_benchmark) {
        //     timer_benchmark_render_.start();
        // } else {
        //     timer_benchmark_render_.stop();
        // }

        config_.render = new_config.render;
        status.config_changed = true;
        status.require_repaint = true;
    }

    if (config_.simulation != new_config.simulation) {
        circuit_store_.set_simulation_config(new_config.simulation);

        config_.simulation = new_config.simulation;
        status.config_changed = true;
        status.require_repaint = true;
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::config() const -> CircuitUIConfig {
    Expects(class_invariant_holds());
    return config_;
}

auto CircuitUIModel::history_status() const -> HistoryStatus {
    Expects(class_invariant_holds());

    if (is_editing_state(config_.state)) {
        const auto& editable_circuit = circuit_store_.editable_circuit();
        return HistoryStatus {
            .undo_available = has_undo(editable_circuit) &&
                              undo_groups_count(editable_circuit) > std::size_t {0},
            .redo_available = has_redo(editable_circuit),
        };
    }
    return HistoryStatus {
        .undo_available = false,
        .redo_available = false,
    };
}

auto CircuitUIModel::allocation_info() const -> CircuitWidgetAllocInfo {
    Expects(class_invariant_holds());

    const auto t = Timer {};

    auto result = CircuitWidgetAllocInfo {
        .circuit_store = circuit_store_.allocation_info(),
        .circuit_renderer = circuit_renderer_.allocation_info(),
    };

    result.collection_time = t.delta();
    return result;
}

auto CircuitUIModel::statistics() const -> Statistics {
    Expects(class_invariant_holds());

    const auto surface_statistics = circuit_renderer_.statistics();
    const auto result = Statistics {
        .simulation_events_per_second = circuit_store_.simulation_events_per_second(),
        .frames_per_second = surface_statistics.frames_per_second,
        .pixel_scale = surface_statistics.pixel_scale,
        .image_size = surface_statistics.image_size,
        .render_mode = last_render_mode_,
    };

    return result;
}

auto CircuitUIModel::do_action(UserAction action) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    switch (action) {
        using enum UserAction;

        case clear_circuit: {
            status |= set_editable_circuit(EditableCircuit {});
            break;
        }
        case reload_circuit: {
            status |= finalize_editing();
            const auto _ = Timer {"Reload Circuit"};
            auto layout = Layout {circuit_store_.layout()};
            // clear circuit to free memory
            status |= do_action(UserAction::clear_circuit);
            status |= set_editable_circuit(EditableCircuit {std::move(layout)});
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
            // status |= this->zoom(+1);
            break;
        }
        case zoom_out: {
            // status |= this->zoom(-1);
            break;
        }
        case reset_view: {
            circuit_renderer_.set_view_point(ViewConfig {}.view_point());
            status.require_repaint = true;
            break;
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::load_circuit_example(int number) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    const auto default_view_point = ViewConfig {}.view_point();
    const auto default_simulation_config = SimulationConfig {};

    // clear circuit to free memory
    status |= do_action(UserAction::clear_circuit);
    status |= set_editable_circuit(load_example_with_logging(number), default_view_point,
                                   default_simulation_config);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::render(BLImage& bl_image, device_pixel_ratio_t device_pixel_ratio)
    -> void {
    Expects(class_invariant_holds());

    // TODO use more device_pixel_ratio_t
    circuit_renderer_.set_device_pixel_ratio(double {device_pixel_ratio});

    if (std::holds_alternative<NonInteractiveState>(config_.state)) {
        circuit_renderer_.render_layout(bl_image, circuit_store_.layout());
    }

    else if (std::holds_alternative<EditingState>(config_.state)) {
        const auto show_size_handles = !editing_logic_manager_.is_area_selection_active();
        circuit_renderer_.render_editable_circuit(
            bl_image, circuit_store_.editable_circuit(), show_size_handles);
    }

    else if (std::holds_alternative<SimulationState>(config_.state)) {
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

auto CircuitUIModel::mouse_press(const MousePressEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // const auto position = get_mouse_position(this, event_);
    // log_mouse_position("mousePressEvent", position, event_);

    if (event.button == MouseButton::Middle) {
        mouse_drag_logic_.mouse_press(event.position);
    }

    if (event.button == MouseButton::Left) {
        const auto position_fine =
            to_grid_fine(event.position, circuit_renderer_.view_config());

        status |= editing_logic_manager_.mouse_press(
            position_fine, circuit_renderer_.view_config(), event.modifiers,
            event.double_click, editable_circuit_pointer(circuit_store_));
    }

    if (event.button == MouseButton::Left && is_simulation(config_.state)) {
        if (const auto point = to_grid(event.position, circuit_renderer_.view_config())) {
            // TODO: status
            circuit_store_.interactive_simulation().mouse_press(*point);
            status.require_repaint = true;
        }
    }

    if (event.button == MouseButton::Right) {
        status |= abort_current_action();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::mouse_move(const MouseMoveEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // const auto position = get_mouse_position(this, event_);
    // log_mouse_position("mouseMoveEvent", position, event_);

    if (event.buttons.is_set(MouseButton::Middle)) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_move(
                                   event.position, circuit_renderer_.view_config()));
        status.require_repaint = true;
    }

    if (event.buttons.is_set(MouseButton::Left)) {
        const auto position_fine =
            to_grid_fine(event.position, circuit_renderer_.view_config());

        status |= editing_logic_manager_.mouse_move(
            position_fine, editable_circuit_pointer(circuit_store_));
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::mouse_release(const MouseReleaseEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // const auto position = get_mouse_position(this, event_);
    // log_mouse_position("mouseReleaseEvent", position, event_);

    if (event.button == MouseButton::Middle) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_release(
                                   event.position, circuit_renderer_.view_config()));
        status.require_repaint = true;
    }

    if (event.button == MouseButton::Left) {
        const auto show_setting_dialog =
            [&](EditableCircuit& editable_circuit,
                std::variant<logicitem_id_t, decoration_id_t> element_id) {
                // TODO:
                // dialog_manager_.show_setting_dialog(editable_circuit, element_id);
                static_cast<void>(editable_circuit);
                static_cast<void>(element_id);
            };

        const auto position_fine =
            to_grid_fine(event.position, circuit_renderer_.view_config());

        status |= editing_logic_manager_.mouse_release(
            position_fine, editable_circuit_pointer(circuit_store_), show_setting_dialog);
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::mouse_wheel(const MouseWheelEvent& event) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // // TODO use actually used value from wheel_scroll_zoom
    // log_mouse_position("wheelEvent", get_mouse_position(this, event_), event_);

    if (const auto view_point =
            circuit_ui_model::wheel_scroll_zoom(event, circuit_renderer_.view_config())) {
        circuit_renderer_.set_view_point(view_point.value());
        status.require_repaint = true;
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::key_press(VirtualKey key) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // Escape
    if (key == VirtualKey::Escape) {
        status |= abort_current_action();
    }

    // Enter
    if (key == VirtualKey::Enter) {
        status |= editing_logic_manager_.confirm_editing(
            editable_circuit_pointer(circuit_store_));

        // TODO: use status
        // some elements might have been deleted (e.g. move-selection confirmation)
        // status |= on_setting_dialog_cleanup_request();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::set_editable_circuit(
    EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point,
    std::optional<SimulationConfig> simulation_config) -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    // finalize_editing();
    // close_all_setting_dialogs();
    circuit_renderer_.reset();

    // disable simulation
    const auto was_simulation = is_simulation(config_.state);
    if (was_simulation) {
        status |= set_circuit_state(*this, NonInteractiveState {});
    }

    // set new circuit
    circuit_store_.set_editable_circuit(std::move(editable_circuit));
    if (view_point) {
        circuit_renderer_.set_view_point(view_point.value());
    }
    if (simulation_config) {
        status |= set_simulation_config(*this, simulation_config.value());
    }

    // re-enable simulation
    if (was_simulation) {
        status |= set_circuit_state(*this, SimulationState {});
    }

    status.require_repaint = true;

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::abort_current_action() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (is_editing_state(config_.state)) {
        // 1) cancel current editing
        if (editing_logic_manager_.is_editing_active()) {
            status |= finalize_editing();
        }

        else {
            // 2) cancel active selection
            if (is_selection_state(config_.state)) {
                circuit_store_.editable_circuit().clear_visible_selection();
                circuit_store_.editable_circuit().finish_undo_group();
                status.require_repaint = true;
            }

            // 3) switch to selection editing mode
            if (is_inserting_state(config_.state)) {
                status |= set_circuit_state(*this, defaults::selection_state);
            }
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::finalize_editing() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    status |=
        editing_logic_manager_.finalize_editing(editable_circuit_pointer(circuit_store_));

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::close_all_setting_dialogs() -> UIStatus {
    Expects(class_invariant_holds());
    auto status = UIStatus {};

    if (is_editing_state(config_.state)) {
        status.dialogs_changed = !dialog_manager_.empty();
        dialog_manager_.close_all(circuit_store_.editable_circuit());
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return status;
}

auto CircuitUIModel::class_invariant_holds() const -> bool {
    // Configs
    Expects(circuit_renderer_.render_config() == config_.render);
    Expects(circuit_store_.simulation_config() == config_.simulation);
    Expects(circuit_store_.circuit_state() == config_.state);
    Expects(editing_logic_manager_.circuit_state() == config_.state);

    // Setting Dialogs
    Expects(is_editing_state(config_.state) || dialog_manager_.empty());

    if (is_editing_state(config_.state) && !editing_logic_manager_.is_editing_active()) {
        // Operation count
        Expects(circuit_store_.editable_circuit().visible_selection_operation_count() ==
                0);

        // History Group
        Expects(!has_ungrouped_undo_entries(circuit_store_.editable_circuit()));

        // History Enabled
        Expects(is_history_enabled(circuit_store_.editable_circuit()));
    }

    return true;
}

auto CircuitUIModel::expensive_invariant_holds() const -> bool {
    // insertion state (expensive so only assert)
    assert(editing_logic_manager_.is_editing_active() ||
           all_normal_display_state(circuit_store_.layout()));

    // editable circuit (expensive so only assert)
    assert(!is_editing_state(config_.state) ||
           is_valid(circuit_store_.editable_circuit()));

    return true;
}

//
// Free Functions
//

auto set_circuit_state(CircuitUIModel& model, CircuitWidgetState value) -> UIStatus {
    auto config = model.config();
    config.state = value;
    return model.set_config(config);
}

auto set_render_config(CircuitUIModel& model, WidgetRenderConfig value) -> UIStatus {
    auto config = model.config();
    config.render = value;
    return model.set_config(config);
}

auto set_simulation_config(CircuitUIModel& model, SimulationConfig value) -> UIStatus {
    auto config = model.config();
    config.simulation = value;
    return model.set_config(config);
}

}  // namespace logicsim
