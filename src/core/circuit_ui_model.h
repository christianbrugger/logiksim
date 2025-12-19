#ifndef LOGICSIM_CORE_CIRCUIT_UI_MODEL_H
#define LOGICSIM_CORE_CIRCUIT_UI_MODEL_H

#include "core/component/circuit_ui_model/circuit_renderer.h"
#include "core/component/circuit_ui_model/circuit_store.h"
#include "core/component/circuit_ui_model/dialog_manager.h"
#include "core/component/circuit_ui_model/mouse_logic/editing_logic_manager.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_drag_logic.h"
#include "core/vocabulary/circuit_ui_config.h"
#include "core/vocabulary/device_pixel_ratio.h"
#include "core/vocabulary/history_status.h"
#include "core/vocabulary/mouse_event.h"
#include "core/vocabulary/render_mode.h"
#include "core/vocabulary/ui_status.h"
#include "core/vocabulary/view_config.h"

#include <gsl/gsl>

#include <filesystem>
#include <optional>

// TODO: remove render mode (direct, buffered) (settings, statistics)
// TODO: use more device_pixel_ratio_t
// TODO: use enum for example int
// TODO: pass device pixel ratio directly to render methods
// TODO: also fix config in circuit-renderer (don't store size & ratio)
// TODO: use -Wconversion for clang & gcc
// TODO: instead of serialize use some history ID to check if circuit needs saving
// TODO: rename vocabulary circuit widget state (anything else that is widget)

namespace logicsim {

class SettingDialogManager;
struct selection_id_t;
struct CircuitWidgetAllocInfo;
class EditableCircuit;
class LoadError;

namespace circuit_ui_model {

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
enum class UserAction : uint8_t {
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

}  // namespace circuit_ui_model

template <>
[[nodiscard]] auto format(circuit_ui_model::UserAction action) -> std::string;

/**
 * @brief: Circuit UI model that hold the circuit and coordinates
 *         managing rendering, simulation and user interactions.
 *
 * Class Invariants:
 *     + configs are the same as for CircuitWidget as all its sub-components
 *     + timer_benchmark_render_ is only active for render_config_.do_benchmark
 *     + timer_run_simulation_ is only active when in simulation state
 *     + setting dialog count is zero if not in editing state
 *     + layout contains only normal display state items if no editing is active
 */
class CircuitUIModel {
   public:
    using Statistics = circuit_ui_model::Statistics;
    using UserAction = circuit_ui_model::UserAction;

   public:
    [[nodiscard]] explicit CircuitUIModel();

    // setter & getters
    [[nodiscard]] auto set_config(const CircuitUIConfig& config) -> UIStatus;
    [[nodiscard]] auto config() const -> CircuitUIConfig;
    [[nodiscard]] auto history_status() const -> HistoryStatus;
    [[nodiscard]] auto allocation_info() const -> CircuitWidgetAllocInfo;
    [[nodiscard]] auto statistics() const -> Statistics;

    [[nodiscard]] auto do_action(UserAction action,
                                 std::optional<point_device_fine_t> position) -> UIStatus;
    // load & save
    [[nodiscard]] auto serialized_circuit() -> std::string;
    [[nodiscard]] auto load_circuit_example(int number) -> UIStatus;
    auto load_circuit(const std::filesystem::path&) -> std::optional<LoadError>;
    auto save_circuit(const std::filesystem::path&) -> bool;
    // render
    auto render(BLImage& bl_image, device_pixel_ratio_t device_pixel_ratio) -> void;

    [[nodiscard]] auto mouse_press(const MousePressEvent& event) -> UIStatus;
    [[nodiscard]] auto mouse_move(const MouseMoveEvent& event) -> UIStatus;
    [[nodiscard]] auto mouse_release(const MouseReleaseEvent& event) -> UIStatus;
    [[nodiscard]] auto mouse_wheel(const MouseWheelEvent& event) -> UIStatus;
    [[nodiscard]] auto key_press(VirtualKey key) -> UIStatus;

   protected:
    // Q_SLOT void on_timer_benchmark_render();
    // Q_SLOT void on_timer_run_simulation();
    // Q_SLOT void on_setting_dialog_cleanup_request();
    // Q_SLOT void on_setting_dialog_attributes_changed(selection_id_t selection_id,
    //                                                  const SettingAttributes&
    //                                                  attributes);

    // auto resizeEvent(QResizeEvent* event_) -> void override;
    // auto renderEvent(BLImage bl_image, device_pixel_ratio_t device_pixel_ratio,
    //                  RenderMode render_mode,
    //                  fallback_info_t fallback_info) -> void override;
    //
    // auto mousePressEvent(QMouseEvent* event_) -> void override;
    // auto mouseMoveEvent(QMouseEvent* event_) -> void override;
    // auto mouseReleaseEvent(QMouseEvent* event_) -> void override;
    //
    // auto wheelEvent(QWheelEvent* event_) -> void override;
    // auto keyPressEvent(QKeyEvent* event_) -> void override;

   private:
    [[nodiscard]] auto set_editable_circuit(
        EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point = {},
        std::optional<SimulationConfig> simulation_config = {}) -> UIStatus;
    [[nodiscard]] auto abort_current_action() -> UIStatus;
    [[nodiscard]] auto finalize_editing() -> UIStatus;
    [[nodiscard]] auto close_all_setting_dialogs() -> UIStatus;

    auto undo() -> void;
    auto redo() -> void;
    auto update_history_status() -> void;
    auto select_all() -> void;
    auto delete_selected() -> void;
    [[nodiscard]] auto copy_paste_position() -> point_t;
    auto copy_selected() -> void;
    auto paste_clipboard() -> void;
    auto zoom(double steps, std::optional<point_device_fine_t> position) -> UIStatus;
    auto log_mouse_position(std::string_view source, point_device_fine_t position)
        -> void;

    [[nodiscard]] auto class_invariant_holds() const -> bool;
    /* only at the end of mutable methods, except paintEvent */
    [[nodiscard]] auto expensive_invariant_holds() const -> bool;

   private:
    // never modify the config directly, call set_config so sub-components are updated
    CircuitUIConfig config_ {};
    HistoryStatus last_history_status_ {};

    circuit_ui_model::CircuitStore circuit_store_ {};
    circuit_ui_model::CircuitRenderer circuit_renderer_ {};
    circuit_ui_model::MouseDragLogic mouse_drag_logic_ {};
    circuit_ui_model::EditingLogicManager editing_logic_manager_;
    circuit_ui_model::DialogManager dialog_manager_;

    bool simulation_image_update_pending_ {false};

    RenderMode last_render_mode_ {RenderMode::buffered};
};

//
// CircuitWidgetState
//

[[nodiscard]] auto set_circuit_state(CircuitUIModel& model, CircuitWidgetState value)
    -> UIStatus;
auto stop_simulation(CircuitUIModel& model) -> void;

//
// RenderConfig
//

[[nodiscard]] auto set_render_config(CircuitUIModel& model, WidgetRenderConfig value)
    -> UIStatus;

// auto set_do_benchmark(CircuitUIModel& model, bool value) -> void;
// auto set_show_circuit(CircuitUIModel& model, bool value) -> void;
// auto set_show_collision_index(CircuitUIModel& model, bool value) -> void;
// auto set_show_connection_index(CircuitUIModel& model, bool value) -> void;
// auto set_show_selection_index(CircuitUIModel& model, bool value) -> void;
//
// auto set_thread_count(CircuitUIModel& model, ThreadCount new_count) -> void;
// auto set_wire_render_style(CircuitUIModel& model, WireRenderStyle style) -> void;
// auto set_direct_rendering(CircuitUIModel& model, bool use_store) -> void;
// auto set_jit_rendering(CircuitUIModel& model, bool enable_jit) -> void;
// auto set_show_render_borders(CircuitUIModel& model, bool value) -> void;
// auto set_show_mouse_position(CircuitUIModel& model, bool value) -> void;

//
// SimulationConfig
//

[[nodiscard]] auto set_simulation_config(CircuitUIModel& model, SimulationConfig value)
    -> UIStatus;

// auto set_simulation_time_rate(CircuitUIModel& model, time_rate_t new_rate) -> void;
// auto set_use_wire_delay(CircuitUIModel& model, bool value) -> void;

}  // namespace logicsim

#endif
