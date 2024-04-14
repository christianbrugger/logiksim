#include "widget/circuit_widget.h"

#include "algorithm/overload.h"
#include "circuit_example.h"
#include "component/circuit_widget/mouse_logic/mouse_wheel_logic.h"
#include "component/circuit_widget/simulation_runner.h"
#include "component/circuit_widget/zoom.h"
#include "copy_paste_clipboard.h"
#include "geometry/scene.h"
#include "load_save_file.h"
#include "logging.h"
#include "qt/clipboard_access.h"
#include "qt/mouse_position.h"
#include "qt/path_conversion.h"
#include "qt/point_conversion.h"
#include "qt/widget_geometry.h"
#include "setting_dialog_manager.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/widget_render_config.h"

namespace logicsim {

namespace {

constexpr auto inline simulation_interval = 20ms;

}

namespace circuit_widget {

auto Statistics::format() const -> std::string {
    return fmt::format(
        "Statistics{{\n"
        "  simulation_events_per_second = {},\n"
        "  frames_per_second = {},\n"
        "  pixel_scale = {},\n"
        "  image_size = {}x{}px\n"
        "  uses_direct_rendering = {},\n"
        "}}",
        simulation_events_per_second, frames_per_second, pixel_scale, image_size.w,
        image_size.h, uses_direct_rendering);
}

}  // namespace circuit_widget

template <>
auto format(circuit_widget::UserAction action) -> std::string {
    switch (action) {
        using enum circuit_widget::UserAction;

        case clear_circuit:
            return "clear_circuit";
        case reload_circuit:
            return "reload_circuit";

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

CircuitWidget::CircuitWidget(QWidget* parent)
    : CircuitWidgetBase(parent),
      editing_logic_manager_ {this},
      setting_dialog_manager_ {new SettingDialogManager {this}} {
    // accept focus so key presses are forwarded to us
    setFocusPolicy(Qt::StrongFocus);

    // initialize components
    circuit_store_.set_simulation_config(simulation_config_);
    circuit_store_.set_circuit_state(circuit_state_);
    render_surface_.set_render_config(render_config_);
    editing_logic_manager_.set_circuit_state(circuit_state_,
                                             editable_circuit_pointer(circuit_store_));

    // timer benchmark rendering
    connect(&timer_benchmark_render_, &QTimer::timeout, this,
            &CircuitWidget::on_timer_benchmark_render);
    if (render_config_.do_benchmark) {
        timer_benchmark_render_.start();
    }

    // timer run simulation
    connect(&timer_run_simulation_, &QTimer::timeout, this,
            &CircuitWidget::on_timer_run_simulation);
    if (is_simulation(circuit_state_)) {
        timer_benchmark_render_.start();
    }

    // setting dialog signals
    connect(setting_dialog_manager_, &SettingDialogManager::request_cleanup, this,
            &CircuitWidget::on_setting_dialog_cleanup_request);
    connect(setting_dialog_manager_, &SettingDialogManager::attributes_changed, this,
            &CircuitWidget::on_setting_dialog_attributes_changed);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::set_render_config(WidgetRenderConfig new_config) -> void {
    Expects(class_invariant_holds());

    if (render_config_ == new_config) {
        return;
    }

    render_surface_.set_render_config(new_config);

    if (new_config.do_benchmark) {
        timer_benchmark_render_.start();
    } else {
        timer_benchmark_render_.stop();
    }

    // update & notify
    render_config_ = new_config;
    emit_render_config_changed(new_config);
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::set_simulation_config(SimulationConfig new_config) -> void {
    Expects(class_invariant_holds());

    if (simulation_config_ == new_config) {
        return;
    }

    circuit_store_.set_simulation_config(new_config);

    // update & notify
    simulation_config_ = new_config;
    emit_simulation_config_changed(new_config);
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::set_circuit_state(CircuitWidgetState new_state) -> void {
    Expects(class_invariant_holds());

    if (circuit_state_ == new_state) {
        return;
    }

    // close dialogs
    if (!is_editing_state(new_state)) {
        close_all_setting_dialogs();
    }

    // finalize editing if needed
    editing_logic_manager_.set_circuit_state(new_state,
                                             editable_circuit_pointer(circuit_store_));

    // clear visible selection
    if (is_selection_state(circuit_state_)) {
        circuit_store_.editable_circuit().clear_visible_selection();
    }

    // circuit store
    circuit_store_.set_circuit_state(new_state);

    // simulation
    if (is_simulation(new_state)) {
        timer_run_simulation_.setInterval(0);
        timer_run_simulation_.start();
    } else {
        timer_run_simulation_.stop();
    }

    // update & notify
    circuit_state_ = new_state;
    emit_circuit_state_changed(new_state);
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::set_editable_circuit(
    EditableCircuit&& editable_circuit__, std::optional<ViewPoint> view_point,
    std::optional<SimulationConfig> simulation_config) -> void {
    Expects(class_invariant_holds());

    finalize_editing();
    close_all_setting_dialogs();
    render_surface_.reset();

    // disable simulation
    const auto was_simulation = is_simulation(circuit_state_);
    if (was_simulation) {
        set_circuit_state(NonInteractiveState {});
    }

    // set new circuit
    circuit_store_.set_editable_circuit(std::move(editable_circuit__));
    if (view_point) {
        render_surface_.set_view_point(view_point.value());
    }
    if (simulation_config) {
        set_simulation_config(simulation_config.value());
    }

    // re-enable simulation
    if (was_simulation) {
        set_circuit_state(SimulationState {});
    }

    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::render_config() const -> WidgetRenderConfig {
    Expects(class_invariant_holds());

    return render_config_;
}

auto CircuitWidget::simulation_config() const -> SimulationConfig {
    Expects(class_invariant_holds());

    return simulation_config_;
}

auto CircuitWidget::circuit_state() const -> CircuitWidgetState {
    Expects(class_invariant_holds());

    return circuit_state_;
}

auto CircuitWidget::serialized_circuit() -> std::string {
    Expects(class_invariant_holds());

    finalize_editing();
    const auto result = serialize_circuit(circuit_store_.layout(), simulation_config_);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return result;
}

auto CircuitWidget::load_circuit_example(int number) -> void {
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

auto CircuitWidget::load_circuit(QString filename) -> bool {
    Expects(class_invariant_holds());

    // store original layout in case load fails
    finalize_editing();
    auto orig_layout__ = Layout {circuit_store_.layout()};
    // clear circuit to free memory
    do_action(UserAction::clear_circuit);

    auto load_result__ = load_circuit_from_file(to_path(filename));
    if (load_result__.success) {
        set_editable_circuit(std::move(load_result__.editable_circuit),
                             load_result__.view_point, load_result__.simulation_config);
    } else {
        set_editable_circuit(EditableCircuit {std::move(orig_layout__)});
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return load_result__.success;
}

auto CircuitWidget::save_circuit(QString filename) -> bool {
    Expects(class_invariant_holds());

    finalize_editing();
    const auto success = save_circuit_to_file(circuit_store_.layout(), to_path(filename),
                                              render_surface_.view_config().view_point(),
                                              simulation_config_);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return success;
}

auto CircuitWidget::statistics() const -> Statistics {
    Expects(class_invariant_holds());

    const auto surface_statistics = render_surface_.statistics();
    const auto result = Statistics {
        .simulation_events_per_second = circuit_store_.simulation_events_per_second(),
        .frames_per_second = surface_statistics.frames_per_second,
        .pixel_scale = surface_statistics.pixel_scale,
        .image_size = surface_statistics.image_size,
        .uses_direct_rendering = surface_statistics.uses_direct_rendering,
    };

    Ensures(class_invariant_holds());
    return result;
}

auto CircuitWidget::do_action(UserAction action) -> void {
    Expects(class_invariant_holds());

    switch (action) {
        using enum UserAction;

        case clear_circuit: {
            set_editable_circuit(EditableCircuit {});
            break;
        }
        case reload_circuit: {
            finalize_editing();
            const auto _ = Timer {"Reload Circuit"};
            auto layout__ = Layout {circuit_store_.layout()};
            // clear circuit to free memory
            do_action(UserAction::clear_circuit);
            set_editable_circuit(EditableCircuit {std::move(layout__)});
            break;
        }

        case select_all: {
            this->select_all();
            break;
        }
        case copy_selected: {
            this->copy_selected();
            break;
        }
        case paste_from_clipboard: {
            this->paste_clipboard();
            break;
        }
        case cut_selected: {
            this->copy_selected();
            this->delete_selected();
            break;
        }
        case delete_selected: {
            this->delete_selected();
            break;
        }

        case zoom_in: {
            render_surface_.set_view_point(
                circuit_widget::zoom(*this, render_surface_.view_config(), +1));
            update();
            break;
        }
        case zoom_out: {
            render_surface_.set_view_point(
                circuit_widget::zoom(*this, render_surface_.view_config(), -1));
            update();
            break;
        }
        case reset_view: {
            render_surface_.set_view_point(ViewConfig {}.view_point());
            update();
            break;
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

void CircuitWidget::on_timer_benchmark_render() {
    Expects(class_invariant_holds());

    update();
}

void CircuitWidget::on_timer_run_simulation() {
    Expects(class_invariant_holds());
    Expects(is_simulation(circuit_state_));

    // force at least one render update between each simulation step
    if (simulation_image_update_pending_) {
        update();
        timer_run_simulation_.setInterval(0);

        Ensures(class_invariant_holds());
        return;
    }
    // otherwise call again at a regular interval
    timer_run_simulation_.setInterval(simulation_interval);

    // run simulation with timeout
    if (circuit_widget::run_simulation(circuit_store_.interactive_simulation(),
                                       realtime_timeout_t {simulation_interval})) {
        simulation_image_update_pending_ = true;
        update();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

Q_SLOT void CircuitWidget::on_setting_dialog_cleanup_request() {
    Expects(class_invariant_holds());

    if (is_editing_state(circuit_state_)) {
        setting_dialog_manager_->run_cleanup(circuit_store_.editable_circuit());
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

Q_SLOT void CircuitWidget::on_setting_dialog_attributes_changed(
    selection_id_t selection_id, SettingAttributes attributes) {
    Expects(class_invariant_holds());

    if (is_editing_state(circuit_state_)) {
        change_setting_attributes(circuit_store_.editable_circuit(), selection_id,
                                  attributes);
        update();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::resizeEvent(QResizeEvent* event_ [[maybe_unused]]) -> void {
    Expects(class_invariant_holds());

    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::paintEvent(QPaintEvent* event_ [[maybe_unused]]) -> void {
    Expects(class_invariant_holds());

    circuit_widget::set_optimal_render_attributes(*this);

    {
        auto context_guard = render_surface_.paintEvent(*this);

        std::visit(
            overload(
                [&](const NonInteractiveState&) {
                    circuit_widget::render_to_context(context_guard.context(),
                                                      render_surface_.render_config(),
                                                      circuit_store_.layout());
                },
                [&](const EditingState&) {
                    const bool show_size_handles =
                        !editing_logic_manager_.is_area_selection_active();

                    circuit_widget::render_to_context(
                        context_guard.context(), render_surface_.render_config(),
                        circuit_store_.editable_circuit(), show_size_handles);
                },
                [&](const SimulationState&) {
                    circuit_widget::render_to_context(
                        context_guard.context(), render_surface_.render_config(),
                        circuit_store_.interactive_simulation().spatial_simulation());
                }),
            circuit_state());
    }

    simulation_image_update_pending_ = false;

    Ensures(class_invariant_holds());
}

auto CircuitWidget::mousePressEvent(QMouseEvent* event_) -> void {
    Expects(class_invariant_holds());

    const auto position = get_mouse_position(this, event_);

    if (event_->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_press(to(position));
        update();
    }

    if (event_->button() == Qt::LeftButton) {
        const auto double_click = event_->type() == QEvent::MouseButtonDblClick;

        if (editing_logic_manager_.mouse_press(
                position, render_surface_.view_config(), event_->modifiers(),
                double_click, editable_circuit_pointer(circuit_store_)) ==
            circuit_widget::ManagerResult::require_update) {
            update();
        }
    }

    if (event_->button() == Qt::LeftButton && is_simulation(circuit_state_)) {
        if (const auto point = to_grid(to(position), render_surface_.view_config())) {
            circuit_store_.interactive_simulation().mouse_press(*point);
            update();
        }
    }

    if (event_->button() == Qt::RightButton) {
        abort_current_action();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::mouseMoveEvent(QMouseEvent* event_) -> void {
    Expects(class_invariant_holds());

    const auto position = get_mouse_position(this, event_);

    if (event_->buttons() & Qt::MiddleButton) {
        set_view_config_offset(
            render_surface_,
            mouse_drag_logic_.mouse_move(to(position), render_surface_.view_config()));
        update();
    }

    if (event_->buttons() & Qt::LeftButton) {
        if (editing_logic_manager_.mouse_move(position, render_surface_.view_config(),
                                              editable_circuit_pointer(circuit_store_)) ==
            circuit_widget::ManagerResult::require_update) {
            update();
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::mouseReleaseEvent(QMouseEvent* event_) -> void {
    Expects(class_invariant_holds());

    const auto position = get_mouse_position(this, event_);

    if (event_->button() == Qt::MiddleButton) {
        set_view_config_offset(
            render_surface_,
            mouse_drag_logic_.mouse_release(to(position), render_surface_.view_config()));
        update();
    }

    if (event_->button() == Qt::LeftButton) {
        const auto show_setting_dialog = [&](EditableCircuit& editable_circuit,
                                             setting_handle_t setting_handle) {
            Expects(setting_dialog_manager_);
            setting_dialog_manager_->show_setting_dialog(editable_circuit,
                                                         setting_handle);
        };

        if (editing_logic_manager_.mouse_release(position, render_surface_.view_config(),
                                                 editable_circuit_pointer(circuit_store_),
                                                 show_setting_dialog) ==
            circuit_widget::ManagerResult::require_update) {
            update();
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::wheelEvent(QWheelEvent* event_) -> void {
    Expects(class_invariant_holds());

    if (const auto view_point = circuit_widget::wheel_scroll_zoom(
            *this, *event_, render_surface_.view_config())) {
        render_surface_.set_view_point(*view_point);
        update();
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::keyPressEvent(QKeyEvent* event_) -> void {
    Expects(class_invariant_holds());

    if (event_->isAutoRepeat()) {
        QWidget::keyPressEvent(event_);
    }

    // Escape
    else if (event_->key() == Qt::Key_Escape) {
        abort_current_action();
    }

    // Enter
    else if (event_->key() == Qt::Key_Enter || event_->key() == Qt::Key_Return) {
        if (editing_logic_manager_.confirm_editing(editable_circuit_pointer(
                circuit_store_)) == circuit_widget::ManagerResult::require_update) {
            update();
            // some elements might have been deleted (e.g. move-selection confirmation)
            on_setting_dialog_cleanup_request();
        }

    }

    else {
        QWidget::keyPressEvent(event_);
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::abort_current_action() -> void {
    Expects(class_invariant_holds());

    if (is_editing_state(circuit_state_)) {
        // 1) cancel current editing
        if (editing_logic_manager_.is_editing_active()) {
            finalize_editing();
        }

        else {
            // 2) cancel active selection
            if (is_selection_state(circuit_state_)) {
                circuit_store_.editable_circuit().clear_visible_selection();
                update();
            }

            // 3) switch to selection editing mode
            if (is_inserting_state(circuit_state_)) {
                set_circuit_state(defaults::selection_state);
            }
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::finalize_editing() -> void {
    Expects(class_invariant_holds());

    editing_logic_manager_.finalize_editing(editable_circuit_pointer(circuit_store_));
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::close_all_setting_dialogs() -> void {
    Expects(class_invariant_holds());

    if (is_editing_state(circuit_state_)) {
        setting_dialog_manager_->close_all(circuit_store_.editable_circuit());
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::select_all() -> void {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        return;
    }
    finalize_editing();
    set_circuit_state(defaults::selection_state);

    visible_selection_select_all(circuit_store_.editable_circuit());
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::delete_selected() -> void {
    Expects(class_invariant_holds());

    if (!is_selection_state(circuit_state_)) {
        return;
    }
    finalize_editing();

    {
        const auto t = Timer {};
        visible_selection_delete_all(circuit_store_.editable_circuit());
        print("Deleted", visible_selection_format(circuit_store_), "in", t);
    }

    update();
    // items with open settings dialogs might have been deleted
    on_setting_dialog_cleanup_request();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::copy_paste_position() const -> point_t {
    Expects(class_invariant_holds());

    const auto result = to_closest_grid_position(to(get_mouse_position(*this)),
                                                 to(get_size_device(*this)),
                                                 render_surface_.view_config());

    Ensures(class_invariant_holds());
    return result;
}

auto CircuitWidget::copy_selected() -> void {
    Expects(class_invariant_holds());

    if (!is_selection_state(circuit_state_)) {
        return;
    }
    finalize_editing();

    const auto t = Timer {};

    const auto copy_position = copy_paste_position();
    if (const auto text = visible_selection_to_clipboard_text(
            circuit_store_.editable_circuit(), copy_position);
        !text.empty()) {
        set_clipboard_text(text);
        print("Copied", visible_selection_format(circuit_store_), "in", t);
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::paste_clipboard() -> void {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        return;
    }

    const auto t = Timer {};

    auto load_result = parse_clipboard_text(get_clipboard_text());
    if (!load_result) {
        Ensures(class_invariant_holds());
        return;
    }

    finalize_editing();
    set_circuit_state(defaults::selection_state);

    const auto paste_position = copy_paste_position();
    auto paste_result = insert_clipboard_data(circuit_store_.editable_circuit(),
                                              load_result.value(), paste_position);

    if (paste_result.is_colliding) {
        editing_logic_manager_.setup_colliding_move(circuit_store_.editable_circuit(),
                                                    std::move(paste_result.cross_points));
    }

    update();
    print("Pasted", visible_selection_format(circuit_store_), "in", t);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::class_invariant_holds() const -> bool {
    // Configs
    Expects(render_surface_.render_config() == render_config_);
    Expects(circuit_store_.simulation_config() == simulation_config_);
    Expects(circuit_store_.circuit_state() == circuit_state_);
    Expects(editing_logic_manager_.circuit_state() == circuit_state_);

    // Timer
    Expects(timer_benchmark_render_.isActive() == render_config_.do_benchmark);
    Expects(timer_run_simulation_.isActive() == is_simulation(circuit_state_));

    // Setting Dialogs
    Expects(is_editing_state(circuit_state_) ||
            setting_dialog_manager_->open_dialog_count() == 0);

    // Visible Selection
    Expects(!is_editing_state(circuit_state_) ||
            circuit_store_.editable_circuit().visible_selection_operation_count() <= 1);

    return true;
}

auto CircuitWidget::expensive_invariant_holds() const -> bool {
    // insertion state (expensive so only assert)
    assert(editing_logic_manager_.is_editing_active() ||
           all_normal_display_state(circuit_store_.layout()));

    // editable circuit (expensive so only assert)
    assert(!is_editing_state(circuit_state_) ||
           is_valid(circuit_store_.editable_circuit()));

    return true;
}

//
// Free Functions
//

auto set_do_benchmark(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.do_benchmark = value;
    circuit_widget.set_render_config(config);
}

auto set_show_circuit(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_circuit = value;
    circuit_widget.set_render_config(config);
}

auto set_show_collision_cache(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_collision_cache = value;
    circuit_widget.set_render_config(config);
}

auto set_show_connection_cache(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_connection_cache = value;
    circuit_widget.set_render_config(config);
}

auto set_show_selection_cache(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_selection_cache = value;
    circuit_widget.set_render_config(config);
}

auto set_thread_count(CircuitWidget& circuit_widget, int new_count) -> void {
    auto config = circuit_widget.render_config();
    config.thread_count = new_count;
    circuit_widget.set_render_config(config);
}

auto set_direct_rendering(CircuitWidget& circuit_widget, bool use_store) -> void {
    auto config = circuit_widget.render_config();
    config.direct_rendering = use_store;
    circuit_widget.set_render_config(config);
}

auto set_simulation_time_rate(CircuitWidget& circuit_widget, time_rate_t new_rate)
    -> void {
    auto config = circuit_widget.simulation_config();
    config.simulation_time_rate = new_rate;
    circuit_widget.set_simulation_config(config);
}

auto set_use_wire_delay(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.simulation_config();
    config.use_wire_delay = value;
    circuit_widget.set_simulation_config(config);
}

auto stop_simulation(CircuitWidget& circuit_widget) -> void {
    if (is_simulation(circuit_widget.circuit_state())) {
        circuit_widget.set_circuit_state(defaults::selection_state);
    }
}

}  // namespace logicsim
