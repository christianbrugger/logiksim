#include "circuit_widget.h"

#include "algorithm/overload.h"
#include "component/circuit_widget/mouse_logic/mouse_wheel_logic.h"
#include "component/circuit_widget/simulation_runner.h"
#include "component/circuit_widget/zoom.h"
#include "default_element_definition.h"
#include "geometry/scene.h"
#include "logging.h"
#include "qt/mouse_position.h"
#include "qt/widget_geometry.h"
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

CircuitWidget::CircuitWidget(QWidget* parent) : CircuitWidgetBase(parent) {
    // initialize components
    render_surface_.set_render_config(render_config_);
    circuit_store_.set_simulation_config(simulation_config_);
    circuit_store_.set_circuit_state(circuit_state_);

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
}

auto CircuitWidget::set_render_config(WidgetRenderConfig new_config) -> void {
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
}

auto CircuitWidget::set_simulation_config(SimulationConfig new_config) -> void {
    if (simulation_config_ == new_config) {
        return;
    }

    circuit_store_.set_simulation_config(new_config);

    // update & notify
    simulation_config_ = new_config;
    emit_simulation_config_changed(new_config);
    update();
}

auto CircuitWidget::set_circuit_state(CircuitWidgetState new_state) -> void {
    if (circuit_state_ == new_state) {
        return;
    }

    circuit_store_.set_circuit_state(new_state);

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
}

auto CircuitWidget::render_config() const -> WidgetRenderConfig {
    return render_config_;
}

auto CircuitWidget::simulation_config() const -> SimulationConfig {
    return simulation_config_;
}

auto CircuitWidget::circuit_state() const -> CircuitWidgetState {
    return circuit_state_;
}

auto CircuitWidget::serialized_circuit() const -> std::string {
    return circuit_widget::serialize_circuit(circuit_store_);
}

auto CircuitWidget::load_circuit_example(int number) -> void {
    const auto default_config = SimulationConfig {};
    circuit_widget::load_circuit_example(circuit_store_, number, default_config);

    render_surface_.reset();
    render_surface_.set_view_point(ViewConfig {}.view_point());
    set_simulation_config(default_config);

    update();
}

auto CircuitWidget::load_circuit(std::string filename) -> bool {
    const auto result = circuit_widget::load_from_file(circuit_store_, filename);

    if (!result.success) {
        return false;
    }

    render_surface_.reset();
    render_surface_.set_view_point(result.view_point);
    set_simulation_config(result.simulation_config);

    update();
    return true;
}

auto CircuitWidget::save_circuit(std::string filename) -> bool {
    // TODO reset mouse logic
    return circuit_widget::save_circuit(circuit_store_, filename,
                                        render_surface_.view_config().view_point());
}

auto CircuitWidget::statistics() const -> Statistics {
    const auto surface_statistics = render_surface_.statistics();

    return Statistics {
        .simulation_events_per_second = circuit_store_.simulation_events_per_second(),
        .frames_per_second = surface_statistics.frames_per_second,
        .pixel_scale = surface_statistics.pixel_scale,
        .image_size = surface_statistics.image_size,
        .uses_direct_rendering = surface_statistics.uses_direct_rendering,
    };
}

auto CircuitWidget::do_action(UserAction action) -> void {
    update();

    switch (action) {
        using enum UserAction;

        case clear_circuit: {
            circuit_widget::set_layout(circuit_store_, Layout {});
            render_surface_.reset();
            return;
        }
        case reload_circuit: {
            circuit_widget::set_layout(circuit_store_, Layout {circuit_store_.layout()});
            render_surface_.reset();
            return;
        }

        case select_all: {
            set_circuit_state(NonInteractiveState {});
            // TODO implement
            return;
        }
        case copy_selected: {
            // TODO implement
            return;
        }
        case paste_from_clipboard: {
            // TODO implement
            return;
        }
        case cut_selected: {
            // TODO implement
            return;
        }
        case delete_selected: {
            // TODO implement
            return;
        }

        case zoom_in: {
            render_surface_.set_view_point(
                circuit_widget::zoom(*this, render_surface_.view_config(), +1));
            return;
        }
        case zoom_out: {
            render_surface_.set_view_point(
                circuit_widget::zoom(*this, render_surface_.view_config(), -1));
            return;
        }
        case reset_view: {
            render_surface_.set_view_point(ViewConfig {}.view_point());
            return;
        }
    }

    std::terminate();
}

void CircuitWidget::on_timer_benchmark_render() {
    update();
}

void CircuitWidget::on_timer_run_simulation() {
    Expects(is_simulation(circuit_state_));

    // force at least one render update between each simulation step
    if (simulation_image_update_pending_) {
        update();
        timer_run_simulation_.setInterval(0);
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
}

auto CircuitWidget::resizeEvent(QResizeEvent* event_ [[maybe_unused]]) -> void {
    update();
}

auto CircuitWidget::paintEvent(QPaintEvent* event_ [[maybe_unused]]) -> void {
    circuit_widget::set_optimal_render_attributes(*this);

    auto& context = render_surface_.begin_paint(backingStore(), get_geometry_info(*this));

    std::visit(overload(
                   [&](const NonInteractiveState& _) {
                       circuit_widget::render_to_context(context,
                                                         render_surface_.render_config(),
                                                         circuit_store_.layout());
                   },
                   [&](const EditingState& _) {
                       bool show_size_handles = false;

                       circuit_widget::render_to_context(
                           context, render_surface_.render_config(),
                           circuit_store_.editable_circuit(), show_size_handles);
                   },
                   [&](const SimulationState& _) {
                       circuit_widget::render_to_context(
                           context, render_surface_.render_config(),
                           circuit_store_.interactive_simulation().spatial_simulation());
                   }),
               circuit_state());

    render_surface_.end_paint(*this);
    simulation_image_update_pending_ = false;
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

auto CircuitWidget::mousePressEvent(QMouseEvent* event_) -> void {
    const auto position = get_mouse_position(this, event_);
    const auto grid_position = to_grid(position, render_surface_.view_config());

    if (event_->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_press(position);
        update();
    }

    else if (event_->button() == Qt::LeftButton) {
        if (!editing_mouse_logic_ && is_editing_state(circuit_state_)) {
            editing_mouse_logic_ =
                get_editing_mouse_logic(position, render_surface_.view_config(),
                                        std::get<EditingState>(circuit_state_));
        }

        if (editing_mouse_logic_) {
            std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                           arg.mouse_press(circuit_store_.editable_circuit(),
                                           grid_position);
                       }},
                       editing_mouse_logic_.value());
            update();
        }
    }
}

auto CircuitWidget::mouseMoveEvent(QMouseEvent* event_) -> void {
    const auto position = get_mouse_position(this, event_);
    const auto grid_position = to_grid(position, render_surface_.view_config());

    if (event_->buttons() & Qt::MiddleButton) {
        set_view_config_offset(
            render_surface_,
            mouse_drag_logic_.mouse_move(position, render_surface_.view_config()));
        update();
    }

    if (editing_mouse_logic_) {
        std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                       arg.mouse_move(circuit_store_.editable_circuit(), grid_position);
                   }},
                   editing_mouse_logic_.value());
        update();
    }
}

auto CircuitWidget::mouseReleaseEvent(QMouseEvent* event_) -> void {
    const auto position = get_mouse_position(this, event_);

    if (event_->button() == Qt::MiddleButton) {
        set_view_config_offset(
            render_surface_,
            mouse_drag_logic_.mouse_release(position, render_surface_.view_config()));

        update();
    }

    if (editing_mouse_logic_) {
        const auto grid_position = to_grid(position, render_surface_.view_config());

        std::visit(overload {[&](circuit_widget::InsertLogicItemLogic& arg) {
                       arg.mouse_release(circuit_store_.editable_circuit(),
                                         grid_position);
                   }},
                   editing_mouse_logic_.value());

        std::visit(
            [&](circuit_widget::EditingLogicInterface& arg) {
                arg.finalize(circuit_store_.editable_circuit());
            },
            editing_mouse_logic_.value());
        editing_mouse_logic_.reset();

        update();
    }
}

auto CircuitWidget::wheelEvent(QWheelEvent* event_) -> void {
    if (const auto view_point = circuit_widget::wheel_scroll_zoom(
            *this, *event_, render_surface_.view_config())) {
        render_surface_.set_view_point(*view_point);
        update();
    }
}

auto CircuitWidget::keyPressEvent(QKeyEvent* event_ [[maybe_unused]]) -> void {
    return;
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
