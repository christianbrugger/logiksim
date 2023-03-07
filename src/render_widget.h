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
#include <QTimer>
#include <QWidget>

#include <chrono>
#include <cmath>
#include <variant>

namespace logicsim {

template <typename... Func>
struct overload : Func... {
    using Func::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

class EmptyMouseLogic {};

class MouseDragLogic {
   public:
    MouseDragLogic(ViewConfig& config) noexcept : config_ {config} {}

    auto mouse_press(QPointF position) -> void {
        last_position = position;
    }

    auto mouse_move(QPointF position) -> void {
        if (last_position.has_value()) {
            config_.offset += to_grid_fine(position, config_)
                              - to_grid_fine(*last_position, config_);
        }
        last_position = position;
    }

    auto mouse_release(QPointF position) -> void {
        mouse_move(position);
        last_position = std::nullopt;
    }

   private:
    ViewConfig& config_;
    std::optional<QPointF> last_position {};
};

class MouseInsertLogic {
   public:
    MouseInsertLogic(EditableCircuit& editable_circuit) noexcept
        : editable_circuit_ {editable_circuit} {}

    auto mouse_press(std::optional<point_t> position) -> void {
        if (position.has_value()) {
            editable_circuit_.add_standard_element(ElementType::or_element, 2, *position);
        }
    }

    auto mouse_move(std::optional<point_t> position) -> void {
        if (position.has_value()) {
            editable_circuit_.add_standard_element(ElementType::or_element, 2, *position);
        }
    }

    auto mouse_release(std::optional<point_t> position) -> void {
        if (position.has_value()) {
            editable_circuit_.add_standard_element(ElementType::or_element, 2, *position);
        }
    }

   private:
    EditableCircuit& editable_circuit_;
};

class WidgetRenderer : public QWidget {
    Q_OBJECT

    using animation_clock = std::chrono::steady_clock;

   public:
    WidgetRenderer(QWidget* parent = nullptr)
        : QWidget(parent),
          last_pixel_ratio_ {devicePixelRatioF()},
          animation_start_ {animation_clock::now()} {
        setAutoFillBackground(false);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAttribute(Qt::WA_NoSystemBackground, true);

        connect(&timer_, &QTimer::timeout, this, &WidgetRenderer::on_timeout);

        render_settings_.view_config.scale = 18;

        reset_circuit();
    }

    auto set_do_benchmark(bool value) -> void {
        do_benchmark_ = value;

        if (value) {
            timer_.start();
        } else {
            timer_.stop();
        }

        update();
    }

    auto set_do_render_circuit(bool value) -> void {
        do_render_circuit_ = value;
        update();
    }

    auto set_do_render_collision_cache(bool value) -> void {
        do_render_collision_cache_ = value;
        update();
    }

    auto set_do_render_connection_cache(bool value) -> void {
        do_render_connection_cache_ = value;
        update();
    }

    auto fps() const -> double {
        return fps_counter_.events_per_second();
    }

    auto scale() const -> double {
        return render_settings_.view_config.scale;
    }

   private:
    auto reset_circuit() -> void {
        circuit_index_ = CircuitIndex {};
        editable_circuit_ = EditableCircuit {circuit_index_.borrow_schematic(circuit_id_),
                                             circuit_index_.borrow_layout(circuit_id_)};

        {
            auto& editable_circuit = editable_circuit_.value();

            auto tree1 = LineTree({point_t {7, 3}, point_t {10, 3}, point_t {10, 1}});
            auto tree2 = LineTree({point_t {10, 3}, point_t {10, 7}, point_t {4, 7},
                                   point_t {4, 4}, point_t {5, 4}});
            // auto tree1 = LineTree({point_t {10, 10}, point_t {10, 12}, point_t {8,
            // 12}}); auto tree2 = LineTree({point_t {10, 12}, point_t {12, 12},
            // point_t {12, 14}});
            auto line_tree = merge({tree1, tree2}).value_or(LineTree {});

            editable_circuit.add_standard_element(ElementType::or_element, 2,
                                                  point_t {5, 3});
            editable_circuit.add_standard_element(ElementType::or_element, 2,
                                                  point_t {15, 6});
            editable_circuit.add_wire(std::move(line_tree));
            editable_circuit.add_wire(LineTree(
                {point_t {8, 1}, point_t {8, 2}, point_t {15, 2}, point_t {15, 4}}));
            // editable_circuit.add_wire(LineTree({point_t {15, 2}, point_t {8, 2}}));
            editable_circuit.swap_and_delete_element(element_id_t {2});

            auto added = editable_circuit.add_standard_element(ElementType::or_element, 9,
                                                               point_t {20, 4});
            fmt::print("added = {}\n", added);

            fmt::print("{}\n", editable_circuit);
            editable_circuit.schematic().validate(Schematic::validate_all);
        }
    }

    Q_SLOT void on_timeout() {
        this->update();
    }

    QSize size_pixels() {
        double ratio = devicePixelRatioF();
        return QSize(width() * ratio, height() * ratio);
    }

    void init() {
        auto window_size = size_pixels();

        qt_image = QImage(window_size.width(), window_size.height(),
                          QImage::Format_ARGB32_Premultiplied);
        qt_image.setDevicePixelRatio(devicePixelRatioF());
        bl_image.createFromData(qt_image.width(), qt_image.height(), BL_FORMAT_PRGB32,
                                qt_image.bits(), qt_image.bytesPerLine());
        bl_info.threadCount = n_threads_;

        fps_counter_.reset();
    };

   protected:
    void resizeEvent(QResizeEvent* event) override {
        if (event == nullptr) {
            return;
        }

        if (event->oldSize() == event->size()) {
            event->accept();
            return;
        }
        init();
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override {
        if (event == nullptr) {
            return;
        }

        if (last_pixel_ratio_ != devicePixelRatioF()) {
            last_pixel_ratio_ = devicePixelRatioF();
            init();
        }
        if (!this->isVisible()) {
            return;
        }

        // build circuit
        auto& editable_circuit = editable_circuit_.value();

        // old behaviour

        /*
        const double animation_seconds
            = std::chrono::duration<double>(animation_clock::now() - animation_start_)
                  .count();
        const double animation_frame = fmod(animation_seconds / 5.0, 1.0);

        Schematic schematic;

        const auto elem0 = schematic.add_element(ElementType::or_element, 2, 1);
        const auto line0 = schematic.add_element(ElementType::wire, 1, 2);
        elem0.output(connection_id_t {0}).connect(line0.input(connection_id_t {0}));
        line0.output(connection_id_t {0}).connect(elem0.input(connection_id_t {1}));

        add_output_placeholders(schematic);
        auto simulation = Simulation {schematic};
        simulation.print_events = true;

        simulation.set_output_delay(elem0.output(connection_id_t {0}), delay_t {10us});
        simulation.set_output_delay(line0.output(connection_id_t {0}), delay_t {40us});
        simulation.set_output_delay(line0.output(connection_id_t {0}), delay_t {60us});
        simulation.set_history_length(line0, delay_t {60us});

        simulation.initialize();
        simulation.submit_event(elem0.input(connection_id_t {0}), 100us, true);
        simulation.submit_event(elem0.input(connection_id_t {0}), 105us, false);
        simulation.submit_event(elem0.input(connection_id_t {0}), 110us, true);
        simulation.submit_event(elem0.input(connection_id_t {0}), 500us, false);

        // TODO use gsl narrow
        auto end_time_ns = static_cast<uint64_t>(90'000 + animation_frame * 120'000);
        const auto end_time [[maybe_unused]] = end_time_ns * 1ns;
        // simulation.run(end_time);

        simulation.run(125us);
        // simulation.run(130us);
        // simulation.run(600us);
        timer_.stop();

        // create layout
        auto layout = Layout {};
        for (auto _ [[maybe_unused]] : range(schematic.element_count())) {
            layout.add_default_element();
        }
        layout.set_position(elem0, point_t {5, 3});

        auto tree1 = LineTree({point_t {10, 10}, point_t {10, 12}, point_t {8, 12}});
        auto tree2 = LineTree({point_t {10, 12}, point_t {12, 12}, point_t {12, 14}});
        auto line_tree = merge({tree1, tree2}).value_or(LineTree {});
        layout.set_line_tree(line0, std::move(line_tree));

        // int w = qt_image.width();
        // int h = qt_image.height();
        */

        bl_ctx.begin(bl_image, bl_info);

        render_background(bl_ctx, render_settings_);

        if (do_render_circuit_) {
            auto simulation = Simulation {editable_circuit.schematic()};
            render_circuit(bl_ctx, editable_circuit.schematic(),
                           editable_circuit.layout(), &simulation, render_settings_);
        }
        if (do_render_collision_cache_) {
            render_editable_circuit_collision_cache(bl_ctx, editable_circuit,
                                                    render_settings_);
        }
        if (do_render_connection_cache_) {
            render_editable_circuit_connection_cache(bl_ctx, editable_circuit,
                                                     render_settings_);
        }

        bl_ctx.end();

        QPainter painter(this);
        painter.drawImage(QPoint(0, 0), qt_image);

        fps_counter_.count_event();
    }

    auto mousePressEvent(QMouseEvent* event) -> void override {
        if (event == nullptr) {
            return;
        }

        // set mouse logic
        if (event->button() == Qt::MiddleButton) {
            mouse_logic_.emplace<MouseDragLogic>(render_settings_.view_config);
        } else if (event->button() == Qt::LeftButton && editable_circuit_.has_value()) {
            mouse_logic_.emplace<MouseInsertLogic>(*editable_circuit_);
        }

        // visit mouse logic
        if (mouse_logic_) {
            const auto grid_position
                = to_grid(event->position(), render_settings_.view_config);

            std::visit(
                overload {
                    [](EmptyMouseLogic& arg) { ; },
                    [&](MouseDragLogic& arg) { arg.mouse_press(event->position()); },
                    [&](MouseInsertLogic& arg) { arg.mouse_press(grid_position); },
                },
                *mouse_logic_);
            update();
        }
    }

    auto mouseMoveEvent(QMouseEvent* event) -> void override {
        if (event == nullptr) {
            return;
        }

        if (mouse_logic_) {
            const auto grid_position
                = to_grid(event->position(), render_settings_.view_config);

            std::visit(
                overload {
                    [&](MouseDragLogic& arg) { arg.mouse_move(event->position()); },
                    [&](MouseInsertLogic& arg) { arg.mouse_move(grid_position); },
                },
                *mouse_logic_);

            update();
        }
    }

    auto mouseReleaseEvent(QMouseEvent* event) -> void override {
        if (event == nullptr) {
            return;
        }

        if (mouse_logic_) {
            const auto grid_position
                = to_grid(event->position(), render_settings_.view_config);

            std::visit(
                overload {
                    [&](MouseDragLogic& arg) { arg.mouse_release(event->position()); },
                    [&](MouseInsertLogic& arg) { arg.mouse_release(grid_position); },
                },
                *mouse_logic_);

            update();
        }

        // delete mouse logic
        mouse_logic_ = std::nullopt;
    }

    auto wheelEvent(QWheelEvent* event) -> void override {
        if (event == nullptr) {
            return;
        }

        const auto standard_delta = 120.0;      // standard delta for one scroll
        const auto standard_zoom_factor = 1.1;  // zoom factor for one scroll
        const auto standard_scroll_pixel = 20;  // pixels to scroll for one scroll

        const auto standard_scroll_grid
            = standard_scroll_pixel / render_settings_.view_config.scale;

        // zoom
        if (event->modifiers() == Qt::ControlModifier) {
            const auto delta = event->angleDelta().y() / standard_delta;
            const auto factor = std::exp(delta * std::log(standard_zoom_factor));

            const auto old_grid_point
                = to_grid_fine(event->position(), render_settings_.view_config);

            render_settings_.view_config.scale *= factor;

            const auto new_grid_point
                = to_grid_fine(event->position(), render_settings_.view_config);

            render_settings_.view_config.offset += new_grid_point - old_grid_point;
            update();
        }

        // standard scroll
        else if (event->modifiers() == Qt::NoModifier) {
            if (event->hasPixelDelta()) {
                // TODO test this
                render_settings_.view_config.offset.x += event->pixelDelta().x();
                render_settings_.view_config.offset.y += event->pixelDelta().y();
            } else {
                render_settings_.view_config.offset.x
                    += standard_scroll_grid * event->angleDelta().x() / standard_delta;
                render_settings_.view_config.offset.y
                    += standard_scroll_grid * event->angleDelta().y() / standard_delta;
            }
            update();
        }

        // inverted scroll
        else if (event->modifiers() == Qt::ShiftModifier) {
            render_settings_.view_config.offset.x
                += standard_scroll_grid * event->angleDelta().y() / standard_delta;
            render_settings_.view_config.offset.y
                += standard_scroll_grid * event->angleDelta().x() / standard_delta;
            update();
        }
    }

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
    std::optional<std::variant<MouseDragLogic, MouseInsertLogic>> mouse_logic_ {};

    // states
    bool do_benchmark_ {false};
    bool do_render_circuit_ {false};
    bool do_render_collision_cache_ {false};
    bool do_render_connection_cache_ {false};

    EventCounter fps_counter_;
};

}  // namespace logicsim

#endif
