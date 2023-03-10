#ifndef LOGIKSIM_RENDER_WIDGET_H
#define LOGIKSIM_RENDER_WIDGET_H

#include "circuit_index.h"
#include "editable_circuit.h"
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

class MouseInsertLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
    };

    MouseInsertLogic(Args args) noexcept;
    ~MouseInsertLogic();

    MouseInsertLogic(const MouseInsertLogic&) = delete;
    MouseInsertLogic(MouseInsertLogic&&) = delete;
    auto operator=(const MouseInsertLogic&) -> MouseInsertLogic& = delete;
    auto operator=(MouseInsertLogic&&) -> MouseInsertLogic& = delete;

    auto mouse_press(std::optional<point_t> position) -> void;
    auto mouse_move(std::optional<point_t> position) -> void;
    auto mouse_release(std::optional<point_t> position) -> void;

   private:
    auto remove_last_element() -> void;
    auto remove_and_insert(std::optional<point_t> position, InsertionMode mode) -> void;

   private:
    EditableCircuit& editable_circuit_;
    element_key_t inserted_key_ {null_element_key};
};

enum class SelectionFunction {
    toggle,
    add,
    substract,
};

// TODO make EditableCircuit part of constructor
class SelectionManager {
   public:
    using selection_mask_t = boost::container::vector<bool>;

    struct operation_t {
        SelectionFunction function;
        rect_fine_t rect;
    };

   public:
    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;

    [[nodiscard]] auto claculate_item_selected(
        element_id_t element_id, const EditableCircuit& editable_circuit) const -> bool;
    [[nodiscard]] auto create_selection_mask(
        const EditableCircuit& editable_circuit) const -> selection_mask_t;
    [[nodiscard]] auto calculate_selected_ids(
        const EditableCircuit& editable_circuit) const -> std::vector<element_id_t>;
    [[nodiscard]] auto calculate_selected_keys(
        const EditableCircuit& editable_circuit) const -> std::vector<element_key_t>;

    auto set_selection(std::vector<element_key_t>&& selected_keys) -> void;
    auto bake_selection(const EditableCircuit& editable_circuit) -> void;
    [[nodiscard]] auto get_baked_selection() const -> const std::vector<element_key_t>&;

   private:
    std::vector<element_key_t> initial_selected_ {};
    std::vector<operation_t> operations_ {};
};

class MouseMoveSelectionLogic {
   public:
    struct Args {
        SelectionManager& manager;
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

   private:
    auto convert_to_temporary() -> void;
    auto apply_current_positions() -> void;
    auto revert_original_positions() -> void;

    SelectionManager& manager_;
    EditableCircuit& editable_circuit_;

    std::optional<point_fine_t> last_position_ {};
    bool converted_ {false};
    bool keep_positions_ {false};
    std::vector<point_t> original_positions_;
};

class MouseSingleSelectionLogic {
   public:
    struct Args {
        SelectionManager& manager;
    };

    MouseSingleSelectionLogic(Args args);

    auto mouse_press(point_fine_t point, Qt::KeyboardModifiers modifiers) -> void;
    auto mouse_move(point_fine_t point) -> void;
    auto mouse_release(point_fine_t point) -> void;

   private:
    SelectionManager& manager_;
};

class MouseAreaSelectionLogic {
   public:
    struct Args {
        QWidget* parent;
        SelectionManager& manager;
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

    SelectionManager& manager_;
    const ViewConfig& view_config_;
    QRubberBand band_;

    std::optional<point_fine_t> first_position_ {};
    bool keep_last_selection_ {false};
};

enum class InteractionState {
    not_interactive,
    select,
    insert,
};

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
    auto scale() const -> double;

   private:
    QSize size_pixels();
    Q_SLOT void on_timeout();
    auto reset_circuit() -> void;
    void init();

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
    RenderSettings render_settings_ {};

    // mouse logic
    SelectionManager selection_manager_ {};
    InteractionState interaction_state_ {InteractionState::not_interactive};
    std::optional<
        std::variant<MouseDragLogic, MouseInsertLogic, MouseSingleSelectionLogic,
                     MouseAreaSelectionLogic, MouseMoveSelectionLogic>>
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

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::InteractionState> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::InteractionState& obj, fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

#endif
