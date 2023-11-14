#ifndef LOGIKSIM_RENDER_WIDGET_H
#define LOGIKSIM_RENDER_WIDGET_H

#include "editable_circuit.h"
#include "event_counter.h"
#include "interactive_simulation.h"
#include "render_circuit.h"
#include "render_widget_base.h"
#include "setting_handle.h"
#include "size_handle.h"
#include "vocabulary/logicitem_definition.h"
#include "vocabulary/simulation_setting.h"

#include <blend2d.h>
#include <gsl/gsl>

#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QRubberBand>
#include <QTimer>
#include <QWidget>

#include <chrono>
#include <cmath>
#include <variant>

namespace logicsim {

class MouseDragLogic {
   public:
    struct Args {
        ViewConfig& view_config;
    };

    MouseDragLogic(Args args) noexcept;

    auto mouse_press(QPointF position) -> void;
    auto mouse_move(QPointF position) -> void;
    auto mouse_release(QPointF position) -> void;

   private:
    ViewConfig& config_;
    std::optional<QPointF> last_position {};
};

class MouseElementInsertLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
        LogicItemDefinition element_definition;
    };

    MouseElementInsertLogic(Args args) noexcept;
    ~MouseElementInsertLogic();

    MouseElementInsertLogic(const MouseElementInsertLogic&) = delete;
    MouseElementInsertLogic(MouseElementInsertLogic&&) = delete;
    auto operator=(const MouseElementInsertLogic&) -> MouseElementInsertLogic& = delete;
    auto operator=(MouseElementInsertLogic&&) -> MouseElementInsertLogic& = delete;

    auto mouse_press(std::optional<point_t> position) -> void;
    auto mouse_move(std::optional<point_t> position) -> void;
    auto mouse_release(std::optional<point_t> position) -> void;

   private:
    auto remove_last_element() -> void;
    auto remove_and_insert(std::optional<point_t> position, InsertionMode mode) -> void;

   private:
    EditableCircuit& editable_circuit_;
    LogicItemDefinition element_definition_;

    selection_old_handle_t temp_element_ {};
};

class MouseLineInsertLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
    };

    MouseLineInsertLogic(Args args) noexcept;
    ~MouseLineInsertLogic();

    MouseLineInsertLogic(const MouseLineInsertLogic&) = delete;
    MouseLineInsertLogic(MouseLineInsertLogic&&) = delete;
    auto operator=(const MouseLineInsertLogic&) -> MouseLineInsertLogic& = delete;
    auto operator=(MouseLineInsertLogic&&) -> MouseLineInsertLogic& = delete;

    auto mouse_press(std::optional<point_t> position) -> void;
    auto mouse_move(std::optional<point_t> position) -> void;
    auto mouse_release(std::optional<point_t> position) -> void;

   private:
    auto remove_last_element() -> void;
    auto remove_and_insert(std::optional<point_t> position, InsertionMode mode) -> void;

   private:
    EditableCircuit& editable_circuit_;
    std::optional<point_t> first_position_ {};
    selection_old_handle_t temp_element_ {};
    std::optional<LineInsertionType> insertion_type_ {};
};

class MouseMoveSelectionLogic {
   public:
    struct Args {
        VisibleSelection& builder;
        EditableCircuit& editable_circuit;
        bool has_colliding {false};
        bool delete_on_cancel {false};
        std::optional<std::vector<point_t>> cross_points {};
    };

    MouseMoveSelectionLogic(Args args);
    ~MouseMoveSelectionLogic();

    MouseMoveSelectionLogic(const MouseMoveSelectionLogic&) = delete;
    MouseMoveSelectionLogic(MouseMoveSelectionLogic&&) = delete;
    auto operator=(const MouseMoveSelectionLogic&) -> MouseMoveSelectionLogic& = delete;
    auto operator=(MouseMoveSelectionLogic&&) -> MouseMoveSelectionLogic& = delete;

    auto mouse_press(point_fine_t point, bool double_click) -> void;
    auto mouse_move(point_fine_t point) -> void;
    auto mouse_release(point_fine_t point) -> void;

    [[nodiscard]] auto finished() -> bool;
    auto confirm() -> void;

   private:
    auto move_selection(point_fine_t point) -> void;

    enum class State {
        waiting_for_first_click,
        move_selection,
        waiting_for_confirmation,
        finished,
        finished_confirmed,
    };

    auto get_selection() -> const Selection&;
    auto copy_selection() -> selection_old_handle_t;

    auto convert_to(InsertionMode mode) -> void;
    auto restore_original_positions() -> void;
    [[nodiscard]] auto calculate_any_element_colliding() -> bool;
    auto delete_selection() -> void;

    VisibleSelection& builder_;
    EditableCircuit& editable_circuit_;
    bool delete_on_cancel_;

    std::optional<point_fine_t> last_position_ {};
    std::pair<int, int> total_offsets_ {};
    InsertionMode insertion_mode_ {InsertionMode::insert_or_discard};
    std::optional<std::vector<point_t>> cross_points_ {};

    State state_ {State::waiting_for_first_click};
};

class MouseSingleSelectionLogic {
   public:
    struct Args {
        VisibleSelection& builder;
        EditableCircuit& editable_circuit;
    };

    MouseSingleSelectionLogic(Args args);

    auto mouse_press(point_fine_t point, bool double_click) -> void;
    auto mouse_move(point_fine_t point) -> void;
    auto mouse_release(point_fine_t point) -> void;

   private:
    VisibleSelection& builder_;
    EditableCircuit& editable_circuit_;
};

class MouseAreaSelectionLogic {
   public:
    struct Args {
        QWidget* parent;
        VisibleSelection& builder;
        const ViewConfig& view_config;
    };

    MouseAreaSelectionLogic(Args args);
    ~MouseAreaSelectionLogic();

    MouseAreaSelectionLogic(const MouseAreaSelectionLogic&) = delete;
    MouseAreaSelectionLogic(MouseAreaSelectionLogic&&) = delete;
    auto operator=(const MouseAreaSelectionLogic&) -> MouseAreaSelectionLogic& = delete;
    auto operator=(MouseAreaSelectionLogic&&) -> MouseAreaSelectionLogic& = delete;

    auto mouse_press(QPointF position, Qt::KeyboardModifiers modifiers) -> void;
    auto mouse_move(QPointF position) -> void;
    auto mouse_release(QPointF position) -> void;

   private:
    auto update_mouse_position(QPointF position) -> void;

    VisibleSelection& builder_;
    const ViewConfig& view_config_;
    QRubberBand band_;

    std::optional<point_fine_t> first_position_ {};
    bool keep_last_selection_ {false};
};

class SimulationInteractionLogic {
   public:
    struct Args {
        InteractiveSimulation& simulation;
    };

    SimulationInteractionLogic(Args args);
    ~SimulationInteractionLogic();

    SimulationInteractionLogic(const SimulationInteractionLogic&) = delete;
    SimulationInteractionLogic(SimulationInteractionLogic&&) = delete;
    auto operator=(const SimulationInteractionLogic&)
        -> SimulationInteractionLogic& = delete;
    auto operator=(SimulationInteractionLogic&&) -> SimulationInteractionLogic& = delete;

    auto mouse_press(std::optional<point_t> point) -> void;
    // auto mouse_move(std::optional<point_t> point) -> void;
    // auto mouse_release(std::optional<point_t> point) -> void;

   private:
    InteractiveSimulation& simulation_;
};

class RendererWidget : public RendererWidgetBase {
    // TODO use Q_OBJECT because of Q_SLOT
    // Q_OBJECT

   public:
    using MouseLogic = std::variant<MouseElementInsertLogic, MouseLineInsertLogic,
                                    MouseSingleSelectionLogic, MouseAreaSelectionLogic,
                                    MouseMoveSelectionLogic, SimulationInteractionLogic,
                                    MouseSizeHandleLogic, MouseSettingHandleLogic>;

   public:
    RendererWidget(QWidget* parent = nullptr);

    auto set_do_benchmark(bool value) -> void;
    auto set_do_render_circuit(bool value) -> void;
    auto set_do_render_collision_cache(bool value) -> void;
    auto set_do_render_connection_cache(bool value) -> void;
    auto set_do_render_selection_cache(bool value) -> void;

    // zero means no threads are used
    auto set_thread_count(int count) -> void;
    auto thread_count() const -> int;
    auto set_use_backing_store(bool value) -> void;
    auto is_using_backing_store() const -> bool;

    auto set_interaction_state(InteractionState state) -> void;
    auto set_simulation_time_rate(time_rate_t time_rate) -> void;
    auto set_use_wire_delay(bool value) -> void;
    [[nodiscard]] auto interaction_state() const -> InteractionState;
    [[nodiscard]] auto simulation_time_rate() const -> time_rate_t;
    [[nodiscard]] auto use_wire_delay() const -> bool;

    // actions
    auto delete_selected_items() -> void;
    auto select_all_items() -> void;
    auto copy_selected_items() -> void;
    auto cut_selected_items() -> void;
    auto paste_clipboard_items() -> void;

    auto fps() const -> double;
    auto simulation_events_per_second() const -> std::optional<double>;
    auto pixel_scale() const -> double;
    auto geometry_toplevel() const -> QRect;
    auto size_device() const -> QSize;
    auto view_config() const noexcept -> const ViewConfig&;

    auto reset_circuit(Layout&& layout = Layout {}) -> void;
    auto reload_circuit() -> void;

    auto save_circuit(std::string filename) -> bool;
    auto serialize_circuit() -> std::string;
    auto load_circuit(std::string filename) -> bool;
    auto load_circuit_example(int id) -> void;

    auto reset_view_config() -> void;
    // negative steps zoom out, positive zoom in
    auto zoom(double steps, std::optional<QPointF> center = {}) -> void;

   private:
    Q_SLOT void on_benchmark_timeout();
    Q_SLOT void on_simulation_timeout();
    auto on_simulation_timeout_impl() -> void;

    // Can only be called inside of paintEvent
    auto init_surface() -> void;
    auto _init_surface_from_backing_store() -> bool;
    auto _init_surface_from_buffer_image() -> void;

    auto reset_interaction_state() -> void;
    auto reset_context() -> void;

    auto set_new_mouse_logic(QMouseEvent* event) -> void;

    auto get_mouse_position(QSinglePointEvent* event) const -> QPointF;
    auto get_mouse_position() -> QPointF;
    auto get_mouse_grid_position() -> point_t;

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    auto mousePressEvent(QMouseEvent* event) -> void override;
    auto mouseMoveEvent(QMouseEvent* event) -> void override;
    auto mouseReleaseEvent(QMouseEvent* event) -> void override;

    auto wheelEvent(QWheelEvent* event) -> void override;
    auto keyPressEvent(QKeyEvent* event) -> void override;

   private:
    qreal last_pixel_ratio_ {-1};
    MouseDragLogic mouse_drag_logic_;

    QImage qt_image_ {};
    CircuitContext context_ {};
    bool is_initialized_ {false};
    bool use_backing_store_ {true};

    QTimer benchmark_timer_;
    QTimer simulation_timer_;
    constexpr static int simulation_timer_interval_ms_ = 20;  // ms
    bool simulation_image_update_requested_ {false};

    // new circuit
    std::optional<EditableCircuit> editable_circuit_ {};

    // simulation
    SimulationConfig simulation_settings_ {};
    std::optional<InteractiveSimulation> simulation_ {};

    // mouse logic
    InteractionState interaction_state_ {InteractionState::not_interactive};
    std::optional<MouseLogic> mouse_logic_ {};

    // setting widgets
    SettingWidgetRegistry setting_widget_registry_ {};

    // states
    bool do_benchmark_ {false};
    bool do_render_circuit_ {false};
    bool do_render_collision_cache_ {false};
    bool do_render_connection_cache_ {false};
    bool do_render_selection_cache_ {false};

    EventCounter fps_counter_ {};
};

}  // namespace logicsim

#endif