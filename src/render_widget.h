
#include <QWidget>
#include <QFrame>
#include <QResizeEvent>
#include <QPainter>

#include <blend2d.h>


namespace logicsim {


    template <typename F, typename G>
    void benchmark_logic_elements(F&& draw_rect, G&& draw_line, int count = 100)
    {
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
                for (auto dy : { 0, 1, 2 }) {
                    int y_pin = y + dy * s;
                    if (dy == 1)
                        draw_line(x, y_pin, x - 0.75 * s, y_pin);
                    draw_line(x + 2 * s, y_pin, x + 2.75 * s, y_pin);
                }
            }
        }

        // double scale = 0.75 + 0.25 * std::sin(0.1 * details.delta_t * 2 * std::numbers::pi);
        // return scale;
    }



    class WidgetRenderer : public QWidget {
        Q_OBJECT
    public:
        WidgetRenderer(QWidget* parent = nullptr) :
            QWidget(parent),
            last_pixel_ratio_(devicePixelRatioF())
        {
            setAutoFillBackground(false);
            setAttribute(Qt::WA_OpaquePaintEvent, true);
            setAttribute(Qt::WA_NoSystemBackground, true);
        }

        QSize size_pixels() {
            double ratio = devicePixelRatioF();
            return QSize(width() * ratio, height() * ratio);
        }

        void init() {
            auto window_size = size_pixels();

            qt_image = QImage(window_size.width(), window_size.height(), QImage::Format_ARGB32_Premultiplied);
            qt_image.setDevicePixelRatio(devicePixelRatioF());
            bl_image.createFromData(qt_image.width(), qt_image.height(), BL_FORMAT_PRGB32, qt_image.bits(), qt_image.bytesPerLine());

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
            // int w = qt_image.width();
            // int h = qt_image.height();

            bl_ctx.begin(bl_image, bl_info);
            renderFrame(bl_ctx);
            bl_ctx.end();

            QPainter painter(this);
            painter.drawImage(QPoint(0, 0), qt_image);
        }

        void renderFrame(BLContext& ctx) {
            ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
            ctx.fillAll();

            BLPath path;
            benchmark_logic_elements(
                [&path](double x, double y, double w, double h) {
                    path.addRect(x, y, w, h);
                },
                [&path](double x1, double y1, double x2, double y2) {
                    path.addLine(BLLine(x1, y1, x2, y2));
                }
            );

            ctx.setFillStyle(BLRgba32(0xFFFFFF00u));
            ctx.setStrokeStyle(BLRgba32(0xFF000000u));
            ctx.setStrokeWidth(2);
            ctx.fillPath(path);
            ctx.strokePath(path);
        }

    private:
        qreal last_pixel_ratio_;

        QImage qt_image;
        BLImage bl_image;

        constexpr static int n_threads_ = 0;
        BLContextCreateInfo bl_info;
        BLContext bl_ctx;
    };

}

