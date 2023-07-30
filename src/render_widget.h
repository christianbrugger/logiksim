#ifndef LOGIKSIM_RENDER_WIDGET_H
#define LOGIKSIM_RENDER_WIDGET_H

#include "circuit_index.h"
#include "editable_circuit/editable_circuit.h"
#include "interactive_simulation.h"
#include "renderer.h"
#include "scene.h"
#include "timer.h"

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

enum class InteractionState {
    not_interactive,
    selection,
    simulation,

    insert_wire,
    insert_button,
    insert_and_element,
    insert_or_element,
    insert_xor_element,
    insert_nand_element,
    insert_nor_element,
    insert_inverter_element,
    insert_flipflop_jk,
    insert_clock_generator,
    insert_shift_register,
};

template <>
auto format(InteractionState type) -> std::string;

[[nodiscard]] auto is_inserting_state(InteractionState state) -> bool;

[[nodiscard]] auto to_logic_item_definition(InteractionState state,
                                            std::size_t default_input_count = 3)
    -> LogicItemDefinition;

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

    selection_handle_t temp_element_ {};
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
    selection_handle_t temp_element_ {};
};

class MouseMoveSelectionLogic {
   public:
    struct Args {
        SelectionBuilder& builder;
        EditableCircuit& editable_circuit;
    };

    MouseMoveSelectionLogic(Args args);
    ~MouseMoveSelectionLogic();

    MouseMoveSelectionLogic(const MouseMoveSelectionLogic&) = delete;
    MouseMoveSelectionLogic(MouseMoveSelectionLogic&&) = delete;
    auto operator=(const MouseMoveSelectionLogic&) -> MouseMoveSelectionLogic& = delete;
    auto operator=(MouseMoveSelectionLogic&&) -> MouseMoveSelectionLogic& = delete;

    auto mouse_press(point_fine_t point) -> void;
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
    auto copy_selection() -> selection_handle_t;

    auto convert_to(InsertionMode mode) -> void;
    auto restore_original_positions() -> void;
    [[nodiscard]] auto calculate_any_element_colliding() -> bool;

    SelectionBuilder& builder_;
    EditableCircuit& editable_circuit_;

    std::optional<point_fine_t> last_position_ {};
    std::pair<int, int> total_offsets_ {};
    InsertionMode insertion_mode_ {InsertionMode::insert_or_discard};

    State state_ {State::waiting_for_first_click};
};

class MouseSingleSelectionLogic {
   public:
    struct Args {
        SelectionBuilder& builder;
    };

    MouseSingleSelectionLogic(Args args);

    auto mouse_press(point_fine_t point) -> void;
    auto mouse_move(point_fine_t point) -> void;
    auto mouse_release(point_fine_t point) -> void;

   private:
    SelectionBuilder& builder_;
};

class MouseAreaSelectionLogic {
   public:
    struct Args {
        QWidget* parent;
        SelectionBuilder& builder;
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

    SelectionBuilder& builder_;
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

class RendererWidget : public QWidget {
    // TODO use Q_OBJECT because of Q_SLOT
    Q_OBJECT

   public:
    RendererWidget(QWidget* parent = nullptr);

    auto set_do_benchmark(bool value) -> void;
    auto set_do_render_circuit(bool value) -> void;
    auto set_do_render_collision_cache(bool value) -> void;
    auto set_do_render_connection_cache(bool value) -> void;
    auto set_do_render_selection_cache(bool value) -> void;

    auto set_interaction_state(InteractionState state) -> void;
    auto set_default_input_count(std::size_t count) -> void;
    auto set_time_rate(time_rate_t time_rate) -> void;
    [[nodiscard]] auto interaction_state() -> InteractionState;
    [[nodiscard]] auto default_input_count() -> std::size_t;
    [[nodiscard]] auto time_rate() -> time_rate_t;

    auto fps() const -> double;
    auto pixel_scale() const -> double;
    auto pixel_size() const -> QSize;

    auto reset_circuit() -> void;
    auto load_circuit(int id) -> void;
    auto reload_circuit() -> void;

    Q_SIGNAL void interaction_state_changed(InteractionState new_state);

   private:
    Q_SLOT void on_benchmark_timeout();
    Q_SLOT void on_simulation_timeout();
    void init();
    auto reset_interaction_state() -> void;

    auto delete_selected_items() -> void;
    auto select_all_items() -> void;
    auto copy_selected_items() -> void;
    auto paste_clipboard_items() -> void;

    auto set_new_mouse_logic(QMouseEvent* event) -> void;

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

    QImage qt_image {};
    BLImage bl_image {};

    constexpr static int n_threads_ {0};
    BLContextCreateInfo bl_info {};
    BLContext bl_ctx {};

    QTimer benchmark_timer_;
    QTimer simulation_timer_;
    constexpr static int simulation_timer_interval_ms_ = 16;  // ms
    time_rate_t time_rate_ {50us};

    // new circuit
    circuit_id_t circuit_id_ {0};
    CircuitIndex circuit_index_ {};
    std::optional<EditableCircuit> editable_circuit_ {};
    std::optional<SelectionBuilder> selection_builder_ {};
    RenderSettings render_settings_ {};

    // simulation
    std::optional<InteractiveSimulation> simulation_;

    // mouse logic
    InteractionState interaction_state_ {InteractionState::not_interactive};
    std::size_t default_input_count_ {3};
    MouseDragLogic mouse_drag_logic_;
    std::optional<std::variant<MouseElementInsertLogic, MouseLineInsertLogic,
                               MouseSingleSelectionLogic, MouseAreaSelectionLogic,
                               MouseMoveSelectionLogic, SimulationInteractionLogic>>
        mouse_logic_ {};

    // states
    bool do_benchmark_ {false};
    bool do_render_circuit_ {false};
    bool do_render_collision_cache_ {false};
    bool do_render_connection_cache_ {false};
    bool do_render_selection_cache_ {false};

    EventCounter fps_counter_;
};

}  // namespace logicsim

#endif
