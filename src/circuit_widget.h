#ifndef LOGICSIM_CIRCUIT_WIDGET_H
#define LOGICSIM_CIRCUIT_WIDGET_H

#include "circuit_widget_base.h"
#include "component/circuit_widget/circuit_store.h"
#include "component/circuit_widget/mouse_logic/editing_logic_manager.h"
#include "component/circuit_widget/mouse_logic/mouse_drag_logic.h"
#include "component/circuit_widget/render_surface.h"

#include <QTimer>

namespace logicsim {

namespace circuit_widget {

/**
 * @brief: Statistics of the Circuit Widget
 */
struct Statistics {
    std::optional<double> simulation_events_per_second;
    double frames_per_second;
    double pixel_scale;
    BLSize image_size;
    bool uses_direct_rendering;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const Statistics&) const -> bool = default;
};

static_assert(std::regular<Statistics>);

/**
 * @brief: Any outside actions that does not require arguments or return values.
 */
enum class UserAction {
    /**
     * @brief: Clears the circuit.
     */
    clear_circuit,

    /**
     * @brief: Reloads the circuit and frees caches. Mostly for debugging purposes.
     */
    reload_circuit,

    select_all,
    copy_selected,
    paste_from_clipboard,
    cut_selected,
    delete_selected,

    zoom_in,
    zoom_out,
    reset_view,
};

}  // namespace circuit_widget

template <>
auto format(circuit_widget::UserAction action) -> std::string;

/**
 * @brief: Widget that hold the circuit and is responsible for managing rendering,
 *         simulation and user interactions.
 *
 * This is a complex class, as it is both an object and called from many
 * different entry points mouse events, top level widgets, and timers.
 * Furthermore it contains several state machines for doing the job
 * over several methods.
 *
 * To tackle this complexity, state machines are separated out to other classes,
 * e.g. mouse logic, render initialization, simulation generation, as much as possible.
 * Those sub components are simple classes and not allowed to generate new Qt events
 * for this widget or themselves. They are only called by use.
 *
 * The remaining complexity of this class is:
 *     + code delegating the work to the components
 *     + code generating follow up events (timer timeouts, render updates)
 *
 * Class Invariants:
 *     + configs are the same as for CircuitWidget as all its sub-components
 *     + timer_benchmark_render_ is only active for WidgetRenderConfig::do_benchmark
 *     + timer_run_simulation_ is only active when in simulation state
 */
class CircuitWidget : public CircuitWidgetBase {
   public:
    using Statistics = circuit_widget::Statistics;
    using UserAction = circuit_widget::UserAction;

   public:
    [[nodiscard]] explicit CircuitWidget(QWidget* parent = nullptr);

    // setter & getters
    auto set_render_config(WidgetRenderConfig new_config) -> void;
    auto set_simulation_config(SimulationConfig new_config) -> void;
    auto set_circuit_state(CircuitWidgetState new_state) -> void;

    [[nodiscard]] auto render_config() const -> WidgetRenderConfig;
    [[nodiscard]] auto simulation_config() const -> SimulationConfig;
    [[nodiscard]] auto circuit_state() const -> CircuitWidgetState;

    // actions without arguments
    auto do_action(UserAction action) -> void;
    // load & save
    [[nodiscard]] auto serialized_circuit() const -> std::string;
    auto load_circuit_example(int number) -> void;
    auto load_circuit(std::string filename) -> bool;
    auto save_circuit(std::string filename) -> bool;

    // statistics
    [[nodiscard]] auto statistics() const -> Statistics;

   protected:
    Q_SLOT void on_timer_benchmark_render();
    Q_SLOT void on_timer_run_simulation();

    auto resizeEvent(QResizeEvent* event_) -> void override;
    auto paintEvent(QPaintEvent* event_) -> void override;

    auto mousePressEvent(QMouseEvent* event_) -> void override;
    auto mouseMoveEvent(QMouseEvent* event_) -> void override;
    auto mouseReleaseEvent(QMouseEvent* event_) -> void override;

    auto wheelEvent(QWheelEvent* event_) -> void override;
    auto keyPressEvent(QKeyEvent* event_) -> void override;

   private:
    auto abort_current_action() -> void;
    auto select_all() -> void;
    auto delete_selected() -> void;

   private:
    // never modify these directly, always call set_* so signals are emmitted
    WidgetRenderConfig render_config_ {};
    SimulationConfig simulation_config_ {};
    CircuitWidgetState circuit_state_ {};

    circuit_widget::CircuitStore circuit_store_ {};
    circuit_widget::RenderSurface render_surface_ {};
    circuit_widget::MouseDragLogic mouse_drag_logic_ {};
    circuit_widget::EditingLogicManager editing_logic_manager_;

    QTimer timer_benchmark_render_ {};
    QTimer timer_run_simulation_ {};
    bool simulation_image_update_pending_ {false};
};

//
// RenderConfig
//

auto set_do_benchmark(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_circuit(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_collision_cache(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_connection_cache(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_selection_cache(CircuitWidget& circuit_widget, bool value) -> void;

auto set_thread_count(CircuitWidget& circuit_widget, int new_count) -> void;
auto set_direct_rendering(CircuitWidget& circuit_widget, bool use_store) -> void;

//
// SimulationConfig
//

auto set_simulation_time_rate(CircuitWidget& circuit_widget, time_rate_t new_rate)
    -> void;
auto set_use_wire_delay(CircuitWidget& circuit_widget, bool value) -> void;

//
// CircuitWidgetState
//

auto stop_simulation(CircuitWidget& circuit_widget) -> void;

}  // namespace logicsim

#endif
