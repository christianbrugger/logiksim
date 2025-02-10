#ifndef LOGICSIM_WIDGET_CIRCUIT_WIDGET_H
#define LOGICSIM_WIDGET_CIRCUIT_WIDGET_H

#include "gui/component/circuit_widget/circuit_renderer.h"
#include "gui/component/circuit_widget/circuit_store.h"
#include "gui/component/circuit_widget/mouse_logic/editing_logic_manager.h"
#include "gui/component/circuit_widget/mouse_logic/mouse_drag_logic.h"
#include "gui/widget/circuit_widget_base.h"

#include "core/vocabulary/fallback_info.h"
#include "core/vocabulary/load_error.h"
#include "core/vocabulary/render_mode.h"
#include "core/vocabulary/setting_attribute.h"
#include "core/vocabulary/thread_count.h"
#include "core/vocabulary/wire_render_style.h"

#include <gsl/gsl>

#include <QTimer>

#include <filesystem>
#include <optional>

namespace logicsim {

class SettingDialogManager;
struct selection_id_t;
struct CircuitWidgetAllocInfo;

namespace circuit_widget {

/**
 * @brief: Statistics of the Circuit Widget
 */
struct Statistics {
    std::optional<double> simulation_events_per_second;
    double frames_per_second;
    double pixel_scale;
    BLSizeI image_size;
    RenderMode render_mode;

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
     * @brief: Reloads the circuit and frees memory. Mostly for debugging purposes.
     */
    reload_circuit,

    undo,
    redo,
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
[[nodiscard]] auto format(circuit_widget::UserAction action) -> std::string;

/**
 * @brief: Widget that hold the circuit and is responsible for managing rendering,
 *         simulation and user interactions.
 *
 * This is a complex class, as it is both an object and called from many
 * different entry points mouse events, top level widgets, and timers.
 * Furthermore it contains several state machines for doing the job
 * across multiple method calls.
 *
 * To tackle this complexity, state machines are separated out to other classes,
 * e.g. mouse logic, render initialization, simulation generation, as much as possible.
 * Those sub components are simple classes and not allowed to generate new Qt events
 * for this widget or themselves. This simplifies the control flow.
 *
 * The remaining complexity of this class is:
 *     + delegating the work to the components
 *     + generating follow up events (timer timeouts, render updates)
 *     + emit signals when internal state change
 *
 * Class Invariants:
 *     + configs are the same as for CircuitWidget as all its sub-components
 *     + timer_benchmark_render_ is only active for render_config_.do_benchmark
 *     + timer_run_simulation_ is only active when in simulation state
 *     + setting dialog count is zero if not in editing state
 *     + layout contains only normal display state items if no editing is active
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
    [[nodiscard]] auto history_status() const -> HistoryStatus;
    [[nodiscard]] auto allocation_info() const -> CircuitWidgetAllocInfo;

    // actions without arguments
    auto do_action(UserAction action) -> void;
    // load & save
    [[nodiscard]] auto serialized_circuit() -> std::string;
    auto load_circuit_example(int number) -> void;
    auto load_circuit(const QString& filename) -> std::optional<LoadError>;
    auto save_circuit(const QString& filename) -> bool;

    // statistics
    [[nodiscard]] auto statistics() const -> Statistics;

   protected:
    Q_SLOT void on_timer_benchmark_render();
    Q_SLOT void on_timer_run_simulation();
    Q_SLOT void on_setting_dialog_cleanup_request();
    Q_SLOT void on_setting_dialog_attributes_changed(selection_id_t selection_id,
                                                     const SettingAttributes& attributes);

    auto resizeEvent(QResizeEvent* event_) -> void override;
    auto renderEvent(BLImage bl_image, device_pixel_ratio_t device_pixel_ratio,
                     RenderMode render_mode,
                     fallback_info_t fallback_info) -> void override;

    auto mousePressEvent(QMouseEvent* event_) -> void override;
    auto mouseMoveEvent(QMouseEvent* event_) -> void override;
    auto mouseReleaseEvent(QMouseEvent* event_) -> void override;

    auto wheelEvent(QWheelEvent* event_) -> void override;
    auto keyPressEvent(QKeyEvent* event_) -> void override;

   private:
    auto set_editable_circuit(
        EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point = {},
        std::optional<SimulationConfig> simulation_config = {}) -> void;
    auto abort_current_action() -> void;
    auto finalize_editing() -> void;
    auto close_all_setting_dialogs() -> void;

    auto undo() -> void;
    auto redo() -> void;
    auto update_history_status() -> void;
    auto select_all() -> void;
    auto delete_selected() -> void;
    [[nodiscard]] auto copy_paste_position() -> point_t;
    auto copy_selected() -> void;
    auto paste_clipboard() -> void;
    auto zoom(double steps) -> void;
    auto log_mouse_position(std::string_view source, QPointF position,
                            QSinglePointEvent* event_ = nullptr) -> void;

    [[nodiscard]] auto class_invariant_holds() const -> bool;
    /* only at the end of mutable methods, except paintEvent */
    [[nodiscard]] auto expensive_invariant_holds() const -> bool;

   private:
    // never modify these directly, always call set_* so signals are emmitted
    WidgetRenderConfig render_config_ {};
    SimulationConfig simulation_config_ {};
    CircuitWidgetState circuit_state_ {};
    HistoryStatus last_history_status_ {};

    circuit_widget::CircuitStore circuit_store_ {};
    circuit_widget::CircuitRenderer circuit_renderer_ {};
    circuit_widget::MouseDragLogic mouse_drag_logic_ {};
    circuit_widget::EditingLogicManager editing_logic_manager_;

    QTimer timer_benchmark_render_ {};
    QTimer timer_run_simulation_ {};
    bool simulation_image_update_pending_ {false};

    RenderMode last_render_mode_ {RenderMode::buffered};
    FallbackPrinter fallback_printer_ {};

    gsl::not_null<SettingDialogManager*> setting_dialog_manager_;
};

//
// RenderConfig
//

auto set_do_benchmark(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_circuit(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_collision_index(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_connection_index(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_selection_index(CircuitWidget& circuit_widget, bool value) -> void;

auto set_thread_count(CircuitWidget& circuit_widget, ThreadCount new_count) -> void;
auto set_wire_render_style(CircuitWidget& circuit_widget, WireRenderStyle style) -> void;
auto set_direct_rendering(CircuitWidget& circuit_widget, bool use_store) -> void;
auto set_jit_rendering(CircuitWidget& circuit_widget, bool enable_jit) -> void;
auto set_show_render_borders(CircuitWidget& circuit_widget, bool value) -> void;
auto set_show_mouse_position(CircuitWidget& circuit_widget, bool value) -> void;

//
// SimulationConfig
//

auto set_simulation_time_rate(CircuitWidget& circuit_widget,
                              time_rate_t new_rate) -> void;
auto set_use_wire_delay(CircuitWidget& circuit_widget, bool value) -> void;

//
// CircuitWidgetState
//

auto stop_simulation(CircuitWidget& circuit_widget) -> void;

}  // namespace logicsim

#endif
