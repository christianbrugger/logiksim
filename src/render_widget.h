#ifndef LOGIKSIM_RENDER_WIDGET_H
#define LOGIKSIM_RENDER_WIDGET_H

#include "circuit_index.h"
#include "editable_circuit.h"
#include "exceptions.h"
#include "layout.h"
#include "range.h"
#include "renderer.h"
#include "schematic.h"
#include "simulation.h"

#include <blend2d.h>
#include <gsl/gsl>

#include <QFrame>
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <QWidget>

#include <chrono>
#include <cmath>

namespace logicsim {

template <typename F, typename G>
void benchmark_logic_elements(F&& draw_rect, G&& draw_line, int count = 100) {
    double s = 10.;  // *details.pixel_ratio / 2;

    double aspect_ratio = 16. / 9.;
    double count_y_d = std::sqrt(count / aspect_ratio);
    int count_x = std::round(count_y_d * aspect_ratio);
    int count_y = std::round(count_y_d);

    for (int i = 0; i < count_x; ++i) {
        for (int j = 0; j < count_y; ++j) {
            double x = 4 * (i + 1) * s;
            double y = 4 * (j + 1) * s;
            draw_rect(x, y + -0.5 * s, 2 * s, 3 * s);
            for (auto dy : {0, 1, 2}) {
                int y_pin = y + dy * s;
                if (dy == 1) draw_line(x, y_pin, x - 0.75 * s, y_pin);
                draw_line(x + 2 * s, y_pin, x + 2.75 * s, y_pin);
            }
        }
    }

    // double scale = 0.75 + 0.25 * std::sin(0.1 * details.delta_t * 2 *
    // std::numbers::pi); return scale;
}

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
        timer_.start();
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
    };

   protected:
    void resizeEvent(QResizeEvent* event) override {
        if (event->oldSize() == event->size()) {
            event->accept();
            return;
        }
        init();
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override {
        if (last_pixel_ratio_ != devicePixelRatioF()) {
            last_pixel_ratio_ = devicePixelRatioF();
            init();
        }
        if (!this->isVisible()) {
            return;
        }

        // build circuit
        auto circuit_index = CircuitIndex {};
        auto circuit_id = circuit_id_t {0};
        auto editable_circuit
            = EditableCircuit {circuit_index.borrow_schematic(circuit_id),
                               circuit_index.borrow_layout(circuit_id)};
        {
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

        // old behaviour

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

        RenderSettings settings {};

        bl_ctx.begin(bl_image, bl_info);
        // renderFrame(bl_ctx);
        render_background(bl_ctx, settings);

        render_circuit(bl_ctx, layout, simulation, settings);
        render_editable_circuit_collision_cache(bl_ctx, editable_circuit, settings);
        render_editable_circuit_connection_cache(bl_ctx, editable_circuit, settings);

        bl_ctx.end();

        QPainter painter(this);
        painter.drawImage(QPoint(0, 0), qt_image);
    }

    void renderFrame(BLContext& ctx) {
        ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
        ctx.fillAll();

        BLPath path;
        benchmark_logic_elements(
            [&path](double x, double y, double w, double h) { path.addRect(x, y, w, h); },
            [&path](double x1, double y1, double x2, double y2) {
                path.addLine(BLLine(x1, y1, x2, y2));
            });

        ctx.setFillStyle(BLRgba32(0xFFFFFF00u));
        ctx.setStrokeStyle(BLRgba32(0xFF000000u));
        ctx.setStrokeWidth(2);
        ctx.fillPath(path);
        ctx.strokePath(path);
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
};

}  // namespace logicsim

#endif
