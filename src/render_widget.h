#ifndef LOGIKSIM_RENDER_WIDGET_H
#define LOGIKSIM_RENDER_WIDGET_H

#include "circuit_index.h"
#include "editable_circuit.h"
#include "exceptions.h"
#include "layout.h"
#include "range.h"
#include "renderer.h"
#include "scene.h"
#include "schematic.h"
#include "simulation.h"
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
    MouseDragLogic(ViewConfig& config) noexcept;

    auto mouse_press(QPointF position) -> void;
    auto mouse_move(QPointF position) -> void;
    auto mouse_release(QPointF position) -> void;

   private:
    ViewConfig& config_;
    std::optional<QPointF> last_position {};
};

class MouseInsertLogic {
   public:
    MouseInsertLogic(EditableCircuit& editable_circuit) noexcept;
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
    add,
    substract,
};

class SelectionManager {
   public:
    using selection_mask_t = boost::container::vector<bool>;

    struct operation_t {
        SelectionFunction function;
        rect_fine_t rect;
        point_fine_t anchor;
    };

   public:
    auto clear() -> void;
    auto add(SelectionFunction function, rect_fine_t rect, point_fine_t anchor) -> void;
    auto update_last(rect_fine_t rect) -> void;

    auto has_selection() const -> bool;
    auto create_selection_mask(const EditableCircuit& editable_circuit) const
        -> selection_mask_t;
    auto last_anchor_position() const -> std::optional<point_fine_t>;

   private:
    std::vector<operation_t> operations_;
};

class MouseSelectionLogic {
   public:
    struct Args {
        QWidget* parent;
        SelectionManager& manager;
        const ViewConfig& view_config;
    };

    MouseSelectionLogic(Args args);

    auto mouse_press(QPointF position, Qt::KeyboardModifiers modifiers) -> void;
    auto mouse_move(QPointF position) -> void;
    auto mouse_release(QPointF position) -> void;

   private:
    auto update_mouse_position(QPointF position) -> void;

    SelectionManager& manager_;
    const ViewConfig& view_config_;
    QRubberBand band_;

    std::optional<point_fine_t> first_position_ {};
};

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

    auto fps() const -> double;
    auto scale() const -> double;

   private:
    QSize size_pixels();
    Q_SLOT void on_timeout();
    auto reset_circuit() -> void;
    void init();

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent([[maybe_unused]] QPaintEvent* event) override;

    auto mousePressEvent(QMouseEvent* event) -> void override;
    auto mouseMoveEvent(QMouseEvent* event) -> void override;
    auto mouseReleaseEvent(QMouseEvent* event) -> void override;

    auto wheelEvent(QWheelEvent* event) -> void override;

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
    std::optional<std::variant<MouseDragLogic, MouseInsertLogic, MouseSelectionLogic>>
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
