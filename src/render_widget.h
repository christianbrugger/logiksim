#ifndef LOGIKSIM_RENDER_WIDGET_H
#define LOGIKSIM_RENDER_WIDGET_H

#include "circuit_index.h"
#include "editable_circuit/editable_circuit.h"
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

enum class InteractionState {
    not_interactive,
    select,
    element_insert,
    line_insert,
};

template <>
auto format(InteractionState type) -> std::string;

class RendererWidget : public QWidget {
    // TODO use Q_OBJECT because of Q_SLOT
    // Q_OBJECT

    using animation_clock = std::chrono::steady_clock;

   public:
    RendererWidget(QWidget* parent = nullptr);

    auto set_do_benchmark(bool value) -> void;
    auto set_do_render_circuit(bool value) -> void;
    auto set_do_render_collision_cache(bool value) -> void;
    auto set_do_render_connection_cache(bool value) -> void;
    auto set_do_render_selection_cache(bool value) -> void;

    auto set_interaction_state(InteractionState state) -> void;

    auto fps() const -> double;
    auto pixel_scale() const -> double;
    auto pixel_size() const -> QSize;

    auto reset_circuit() -> void;
    auto load_circuit(int id) -> void;
    auto reload_circuit() -> void;

   private:
    Q_SLOT void on_timeout();
    void init();
    auto reset_interaction_state() -> void;

    auto delete_selected_items() -> void;
    auto select_all_items() -> void;
    auto set_new_mouse_logic(QMouseEvent* event) -> void;

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent([[maybe_unused]] QPaintEvent* event) override;

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

    QTimer timer_;
    animation_clock::time_point animation_start_;

    // new circuit
    circuit_id_t circuit_id_ {0};
    CircuitIndex circuit_index_ {};
    std::optional<EditableCircuit> editable_circuit_ {};
    std::optional<SelectionBuilder> selection_builder_ {};
    RenderSettings render_settings_ {};

    // mouse logic
    InteractionState interaction_state_ {InteractionState::not_interactive};
    MouseDragLogic mouse_drag_logic_;
    std::optional<std::variant<MouseElementInsertLogic, MouseLineInsertLogic,
                               MouseSingleSelectionLogic, MouseAreaSelectionLogic,
                               MouseMoveSelectionLogic>>
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
