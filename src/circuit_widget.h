#ifndef LOGICSIM_CIRCUIT_WIDGET_H
#define LOGICSIM_CIRCUIT_WIDGET_H

#include "circuit_widget_base.h"

#include <blend2d.h>

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

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const Statistics&) const -> bool = default;
};

static_assert(std::regular<Statistics>);

/**
 * @brief: Actions from Buttons or Hotkeys
 */
enum class UserAction {
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
 */
class CircuitWidget : public CircuitWidgetBase {
    // TODO can we use Q_OBJECT directly here ?

   public:
    using Statistics = circuit_widget::Statistics;
    using UserAction = circuit_widget::UserAction;

   public:
    CircuitWidget(QWidget* parent = nullptr);

    // setter & getters
    auto set_render_config(WidgetRenderConfig new_config) -> void;
    auto set_simulation_config(SimulationConfig new_config) -> void;
    auto set_circuit_state(CircuitWidgetState new_state) -> void;

    auto render_config() const -> WidgetRenderConfig;
    auto simulation_config() const -> SimulationConfig;
    auto circuit_state() const -> CircuitWidgetState;

    // load & save
    auto serialized_circuit() const -> std::string;
    auto load_new_circuit() -> void;
    auto load_circuit_example(int) -> void;
    auto load_circuit(std::string filename) -> bool;
    auto save_circuit(std::string filename) -> bool;
    auto reload_circuit() -> void;

    // statistics
    auto statistics() const -> Statistics;

    // actions
    auto submit_user_action(UserAction action) -> void;

   private:
    // never modify these directly, always call set_*, also for internal changes
    WidgetRenderConfig render_config_;
    SimulationConfig simulation_config_;
    CircuitWidgetState circuit_state_;
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

}  // namespace logicsim

#endif
