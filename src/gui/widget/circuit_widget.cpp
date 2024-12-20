#include "gui/widget/circuit_widget.h"

#include "gui/component/circuit_widget/mouse_logic/mouse_logic_result.h"
#include "gui/component/circuit_widget/mouse_logic/mouse_wheel_logic.h"
#include "gui/component/circuit_widget/simulation_runner.h"
#include "gui/component/circuit_widget/zoom.h"
#include "gui/format/qt_type.h"
#include "gui/qt/clipboard_access.h"
#include "gui/qt/mouse_position.h"
#include "gui/qt/path_conversion.h"
#include "gui/qt/point_conversion.h"
#include "gui/qt/widget_geometry.h"
#include "gui/widget/setting_dialog_manager.h"

#include "core/circuit_example.h"
#include "core/copy_paste_clipboard.h"
#include "core/geometry/scene.h"
#include "core/load_save_file.h"
#include "core/logging.h"
#include "core/vocabulary/allocation_info.h"
#include "core/vocabulary/device_pixel_ratio.h"
#include "core/vocabulary/simulation_config.h"
#include "core/vocabulary/widget_render_config.h"

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
        "  render_mode = {},\n"
        "}}",
        simulation_events_per_second, frames_per_second, pixel_scale, image_size.w,
        image_size.h, render_mode);
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

CircuitWidget::CircuitWidget(QWidget* parent)
    : CircuitWidgetBase(parent),
      editing_logic_manager_ {this},
      setting_dialog_manager_ {new SettingDialogManager {this}} {
    // accept focus so key presses are forwarded to us
    setFocusPolicy(Qt::StrongFocus);

    // initialize components
    circuit_store_.set_simulation_config(simulation_config_);
    circuit_store_.set_circuit_state(circuit_state_);
    circuit_renderer_.set_render_config(render_config_);
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

    circuit_renderer_.set_render_config(new_config);
    // TODO remove direct rendering from this config
    this->set_requested_render_mode(new_config.direct_rendering ? RenderMode::direct
                                                                : RenderMode::buffered);

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
        circuit_store_.editable_circuit().finish_undo_group();
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
    EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point,
    std::optional<SimulationConfig> simulation_config) -> void {
    Expects(class_invariant_holds());

    finalize_editing();
    close_all_setting_dialogs();
    circuit_renderer_.reset();

    // disable simulation
    const auto was_simulation = is_simulation(circuit_state_);
    if (was_simulation) {
        set_circuit_state(NonInteractiveState {});
    }

    // set new circuit
    circuit_store_.set_editable_circuit(std::move(editable_circuit));
    if (view_point) {
        circuit_renderer_.set_view_point(view_point.value());
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

auto CircuitWidget::history_status() const -> HistoryStatus {
    Expects(class_invariant_holds());

    if (is_editing_state(circuit_state_)) {
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

auto CircuitWidget::allocation_info() const -> CircuitWidgetAllocInfo {
    Expects(class_invariant_holds());

    const auto t = Timer {};

    auto result = CircuitWidgetAllocInfo {
        .circuit_store = circuit_store_.allocation_info(),
        .circuit_renderer = circuit_renderer_.allocation_info(),
    };

    result.collection_time = t.delta();
    return result;
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

auto CircuitWidget::load_circuit(const QString& filename) -> std::optional<LoadError> {
    Expects(class_invariant_holds());

    // store original layout in case load fails
    finalize_editing();
    auto orig_layout = Layout {circuit_store_.layout()};
    // clear circuit to free memory
    do_action(UserAction::clear_circuit);

    auto load_result = load_circuit_from_file(to_path(filename));

    if (load_result.has_value()) {
        set_editable_circuit(std::move(load_result->editable_circuit),
                             load_result->view_point, load_result->simulation_config);
    } else {
        set_editable_circuit(EditableCircuit {std::move(orig_layout)});
    }

    const auto result = load_result  //
                            ? std::nullopt
                            : std::make_optional(std::move(load_result.error()));

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return result;
}

auto CircuitWidget::save_circuit(const QString& filename) -> bool {
    Expects(class_invariant_holds());

    finalize_editing();
    const auto success = save_circuit_to_file(
        circuit_store_.layout(), to_path(filename),
        circuit_renderer_.view_config().view_point(), simulation_config_);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
    return success;
}

auto CircuitWidget::statistics() const -> Statistics {
    Expects(class_invariant_holds());

    const auto surface_statistics = circuit_renderer_.statistics();
    const auto result = Statistics {
        .simulation_events_per_second = circuit_store_.simulation_events_per_second(),
        .frames_per_second = surface_statistics.frames_per_second,
        .pixel_scale = surface_statistics.pixel_scale,
        .image_size = surface_statistics.image_size,
        .render_mode = last_render_mode_,
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
            auto layout = Layout {circuit_store_.layout()};
            // clear circuit to free memory
            do_action(UserAction::clear_circuit);
            set_editable_circuit(EditableCircuit {std::move(layout)});
            break;
        }

        case undo: {
            this->undo();
            break;
        }
        case redo: {
            this->redo();
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
            this->zoom(+1);
            break;
        }
        case zoom_out: {
            this->zoom(-1);
            break;
        }
        case reset_view: {
            circuit_renderer_.set_view_point(ViewConfig {}.view_point());
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
    selection_id_t selection_id, const SettingAttributes& attributes) {
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

auto CircuitWidget::renderEvent(BLImage bl_image, device_pixel_ratio_t device_pixel_ratio,
                                RenderMode render_mode,
                                fallback_info_t fallback_info) -> void {
    Expects(class_invariant_holds());

    fallback_printer_.print_if_set("WARNING: Cannot use direct rendering:",
                                   fallback_info);

    // TODO use more device_pixel_ratio_t
    circuit_renderer_.set_device_pixel_ratio(double {device_pixel_ratio});

    if (std::holds_alternative<NonInteractiveState>(circuit_state_)) {
        circuit_renderer_.render_layout(bl_image, circuit_store_.layout());
    }

    else if (std::holds_alternative<EditingState>(circuit_state_)) {
        const bool show_size_handles = !editing_logic_manager_.is_area_selection_active();
        circuit_renderer_.render_editable_circuit(
            bl_image, circuit_store_.editable_circuit(), show_size_handles);
    }

    else if (std::holds_alternative<SimulationState>(circuit_state_)) {
        circuit_renderer_.render_simulation(
            bl_image, circuit_store_.interactive_simulation().spatial_simulation());
    }

    else {
        std::terminate();
    }

    last_render_mode_ = render_mode;
    simulation_image_update_pending_ = false;

    // TODO update history status in other methods as history is changed
    // and also add it to the invariant to make sure it is up to date
    // for now update it only here, until refactoring this class into core
    update_history_status();

    Ensures(class_invariant_holds());
}

auto CircuitWidget::mousePressEvent(QMouseEvent* event_) -> void {
    Expects(class_invariant_holds());

    const auto position = get_mouse_position(this, event_);
    log_mouse_position("mousePressEvent", position, event_);

    if (event_->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_press(to(position));
        update();
    }

    if (event_->button() == Qt::LeftButton) {
        const auto double_click = event_->type() == QEvent::MouseButtonDblClick;

        if (editing_logic_manager_
                .mouse_press(position, circuit_renderer_.view_config(),
                             event_->modifiers(), double_click,
                             editable_circuit_pointer(circuit_store_))
                .require_update) {
            update();
        }
    }

    if (event_->button() == Qt::LeftButton && is_simulation(circuit_state_)) {
        if (const auto point = to_grid(to(position), circuit_renderer_.view_config())) {
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
    log_mouse_position("mouseMoveEvent", position, event_);

    if ((event_->buttons() & Qt::MiddleButton) != 0) {
        set_view_config_offset(
            circuit_renderer_,
            mouse_drag_logic_.mouse_move(to(position), circuit_renderer_.view_config()));
        update();
    }

    if ((event_->buttons() & Qt::LeftButton) != 0) {
        if (editing_logic_manager_
                .mouse_move(position, circuit_renderer_.view_config(),
                            editable_circuit_pointer(circuit_store_))
                .require_update) {
            update();
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::mouseReleaseEvent(QMouseEvent* event_) -> void {
    Expects(class_invariant_holds());

    const auto position = get_mouse_position(this, event_);
    log_mouse_position("mouseReleaseEvent", position, event_);

    if (event_->button() == Qt::MiddleButton) {
        set_view_config_offset(circuit_renderer_,
                               mouse_drag_logic_.mouse_release(
                                   to(position), circuit_renderer_.view_config()));
        update();
    }

    if (event_->button() == Qt::LeftButton) {
        const auto show_setting_dialog =
            [&](EditableCircuit& editable_circuit,
                std::variant<logicitem_id_t, decoration_id_t> element_id) {
                Expects(setting_dialog_manager_);
                setting_dialog_manager_->show_setting_dialog(editable_circuit,
                                                             element_id);
            };

        const auto result = editing_logic_manager_.mouse_release(
            position, circuit_renderer_.view_config(),
            editable_circuit_pointer(circuit_store_), show_setting_dialog);

        if (result.require_update) {
            update();
        }
        if (result.inserted_decoration) {
            set_circuit_state(defaults::selection_state);
            circuit_store_.editable_circuit().reopen_undo_group();
            circuit_store_.editable_circuit().set_visible_selection(
                Selection {{}, std::array {result.inserted_decoration}});
            circuit_store_.editable_circuit().finish_undo_group();
        }
    }

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::wheelEvent(QWheelEvent* event_) -> void {
    Expects(class_invariant_holds());

    // TODO use actually used value from wheel_scroll_zoom
    log_mouse_position("wheelEvent", get_mouse_position(this, event_), event_);

    if (const auto view_point = circuit_widget::wheel_scroll_zoom(
            *this, *event_, circuit_renderer_.view_config())) {
        circuit_renderer_.set_view_point(*view_point);
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
        if (editing_logic_manager_
                .confirm_editing(editable_circuit_pointer(circuit_store_))
                .require_update) {
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
                circuit_store_.editable_circuit().finish_undo_group();
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

auto CircuitWidget::undo() -> void {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        return;
    }
    finalize_editing();
    close_all_setting_dialogs();
    set_circuit_state(defaults::selection_state);

    circuit_store_.editable_circuit().undo_group();
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::redo() -> void {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        return;
    }
    finalize_editing();
    close_all_setting_dialogs();
    set_circuit_state(defaults::selection_state);

    circuit_store_.editable_circuit().redo_group();
    update();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::update_history_status() -> void {
    Expects(class_invariant_holds());

    const auto status = history_status();

    if (status != last_history_status_) {
        last_history_status_ = status;
        emit_history_status_changed(status);
    }

    Ensures(class_invariant_holds());
}

auto CircuitWidget::select_all() -> void {
    Expects(class_invariant_holds());

    if (!is_editing_state(circuit_state_)) {
        return;
    }
    finalize_editing();
    set_circuit_state(defaults::selection_state);

    visible_selection_select_all(circuit_store_.editable_circuit());
    circuit_store_.editable_circuit().finish_undo_group();
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

    circuit_store_.editable_circuit().finish_undo_group();
    update();
    // items with open settings dialogs might have been deleted
    on_setting_dialog_cleanup_request();

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

auto CircuitWidget::copy_paste_position() -> point_t {
    Expects(class_invariant_holds());

    const auto position = get_mouse_position(*this);
    const auto result = to_closest_grid_position(to(position), to(get_size_device(*this)),
                                                 circuit_renderer_.view_config());

    log_mouse_position("copy_paste_position", position);

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
        print("WARNING: Unable to paste clipboard data.");
        print(load_result.error().type());
        print(load_result.error().format());
        print();

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
    } else {
        circuit_store_.editable_circuit().finish_undo_group();
    }

    update();
    print("Pasted", visible_selection_format(circuit_store_), "in", t);

    Ensures(class_invariant_holds());
    Ensures(expensive_invariant_holds());
}

/**
 * @brief: Find a position within the widget and zoom by given steps.
 *
 * Note, this the current mouse position or the center of the widget, if the
 * mouse is outside of the widget.
 */
auto CircuitWidget::zoom(double steps) -> void {
    const auto center = get_mouse_position_inside_widget(*this);
    log_mouse_position("zoom", center);

    circuit_renderer_.set_view_point(
        circuit_widget::zoom(circuit_renderer_.view_config(), steps, to(center)));

    update();
}

auto CircuitWidget::log_mouse_position(std::string_view source, QPointF position,
                                       QSinglePointEvent* event_) -> void {
    if (render_config_.show_mouse_position) {
        circuit_renderer_.set_mouse_position_info(
            create_mouse_position_info(source, position, event_));
    } else {
        circuit_renderer_.set_mouse_position_info(std::nullopt);
    }
}

auto CircuitWidget::class_invariant_holds() const -> bool {
    // Configs
    Expects(circuit_renderer_.render_config() == render_config_);
    Expects(circuit_store_.simulation_config() == simulation_config_);
    Expects(circuit_store_.circuit_state() == circuit_state_);
    Expects(editing_logic_manager_.circuit_state() == circuit_state_);
    Expects(circuit_renderer_.render_config().direct_rendering ==
            (this->requested_render_mode() == RenderMode::direct));

    // Timer
    Expects(timer_benchmark_render_.isActive() == render_config_.do_benchmark);
    Expects(timer_run_simulation_.isActive() == is_simulation(circuit_state_));

    // Setting Dialogs
    Expects(is_editing_state(circuit_state_) ||
            setting_dialog_manager_->open_dialog_count() == 0);

    if (is_editing_state(circuit_state_) && !editing_logic_manager_.is_editing_active()) {
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

auto set_thread_count(CircuitWidget& circuit_widget, ThreadCount new_count) -> void {
    auto config = circuit_widget.render_config();
    config.thread_count = new_count;
    circuit_widget.set_render_config(config);
}

auto set_wire_render_style(CircuitWidget& circuit_widget, WireRenderStyle style) -> void {
    auto config = circuit_widget.render_config();
    config.wire_render_style = style;
    circuit_widget.set_render_config(config);
}

auto set_direct_rendering(CircuitWidget& circuit_widget, bool use_store) -> void {
    auto config = circuit_widget.render_config();
    config.direct_rendering = use_store;
    circuit_widget.set_render_config(config);
}

auto set_jit_rendering(CircuitWidget& circuit_widget, bool enable_jit) -> void {
    auto config = circuit_widget.render_config();
    config.jit_rendering = enable_jit;
    circuit_widget.set_render_config(config);
}

auto set_show_render_borders(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_render_borders = value;
    circuit_widget.set_render_config(config);
}

auto set_show_mouse_position(CircuitWidget& circuit_widget, bool value) -> void {
    auto config = circuit_widget.render_config();
    config.show_mouse_position = value;
    circuit_widget.set_render_config(config);
}

auto set_simulation_time_rate(CircuitWidget& circuit_widget,
                              time_rate_t new_rate) -> void {
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
