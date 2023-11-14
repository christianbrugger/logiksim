#include "circuit_widget.h"

#include "logging.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/widget_render_config.h"

#include <exception>

namespace logicsim {

namespace circuit_widget {

auto Statistics::format() const -> std::string {
    return fmt::format(
        "Statistics{{\n"
        "  simulation_events_per_second = {},\n"
        "  frames_per_second = {},\n"
        "  pixel_scale = {},\n"
        "  image_size = {}x{}px\n"
        "}}",
        simulation_events_per_second, frames_per_second, pixel_scale, image_size.w,
        image_size.h);
}

}  // namespace circuit_widget

template <>
auto format(circuit_widget::UserAction action) -> std::string {
    switch (action) {
        using enum circuit_widget::UserAction;

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

CircuitWidget::CircuitWidget(QWidget* parent) : CircuitWidgetBase(parent) {}

auto CircuitWidget::set_render_config(WidgetRenderConfig new_config) -> void {
    if (render_config_ == new_config) {
        return;
    }

    render_surface_.set_render_config(new_config);

    // update & notify
    render_config_ = new_config;
    emit_render_config_changed(new_config);
    update();
}

auto CircuitWidget::set_simulation_config(SimulationConfig new_config) -> void {
    if (simulation_config_ == new_config) {
        return;
    }

    // update & notify
    simulation_config_ = new_config;
    emit_simulation_config_changed(new_config);
    update();
}

auto CircuitWidget::set_circuit_state(CircuitWidgetState new_state) -> void {
    if (circuit_state_ == new_state) {
        return;
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
    // TODO implement
    return std::string();
}

auto CircuitWidget::load_new_circuit() -> void {
    // TODO implement
}

auto CircuitWidget::load_circuit_example(int) -> void {
    // TODO implement
}

auto CircuitWidget::load_circuit(std::string filename) -> bool {
    // TODO implement
    return true;
}

auto CircuitWidget::save_circuit(std::string filename) -> bool {
    // TODO implement
    return true;
}

auto CircuitWidget::reload_circuit() -> void {
    // TODO implement
}

auto CircuitWidget::statistics() const -> Statistics {
    const auto surface_statistics = render_surface_.statistics();

    return Statistics {
        .simulation_events_per_second = {},  // TODO implement
        .frames_per_second = surface_statistics.frames_per_second,
        .pixel_scale = surface_statistics.pixel_scale,
        .image_size = surface_statistics.image_size,
    };
}

auto CircuitWidget::submit_user_action(UserAction action) -> void {
    switch (action) {
        using enum UserAction;

        case select_all: {
            return;
        }
        case copy_selected: {
            return;
        }
        case paste_from_clipboard: {
            return;
        }
        case cut_selected: {
            return;
        }
        case delete_selected: {
            return;
        }

        case zoom_in: {
            return;
        }
        case zoom_out: {
            return;
        }
        case reset_view: {
            const auto config = ViewConfig {};
            render_surface_.set_view_config_offset(config.offset());
            render_surface_.set_view_config_device_scale(config.device_scale());
            update();
            return;
        }
    }

    std::terminate();
}

auto CircuitWidget::resizeEvent(QResizeEvent* event_) -> void {
    render_surface_.resizeEvent(*this, event_);
    update();
}

auto CircuitWidget::paintEvent(QPaintEvent* event_) -> void {
    render_surface_.paintEvent(*this, event_, nullptr, nullptr, false);
}

namespace {

auto get_mouse_position(QWidget* widget, QSinglePointEvent* event_) -> QPointF {
    Expects(widget);
    Expects(event_);

    return widget->mapFromGlobal(event_->globalPosition());
}

}  // namespace

auto CircuitWidget::mousePressEvent(QMouseEvent* event_) -> void {
    const auto position = get_mouse_position(this, event_);

    if (event_->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_press(position);
        update();
    }
}

auto CircuitWidget::mouseMoveEvent(QMouseEvent* event_) -> void {
    const auto position = get_mouse_position(this, event_);

    if (event_->buttons() & Qt::MiddleButton) {
        const auto new_offset =
            mouse_drag_logic_.mouse_move(position, render_surface_.view_config());
        render_surface_.set_view_config_offset(new_offset);
        update();
    }
}

auto CircuitWidget::mouseReleaseEvent(QMouseEvent* event_) -> void {
    const auto position = get_mouse_position(this, event_);

    if (event_->button() == Qt::MiddleButton) {
        const auto new_offset =
            mouse_drag_logic_.mouse_release(position, render_surface_.view_config());
        render_surface_.set_view_config_offset(new_offset);
        update();
    }
}

auto CircuitWidget::wheelEvent(QWheelEvent* event_) -> void {
    return;
}

auto CircuitWidget::keyPressEvent(QKeyEvent* event_) -> void {
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

}  // namespace logicsim
