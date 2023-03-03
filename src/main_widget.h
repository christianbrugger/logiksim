
#include "render_widget.h"

#include <gsl/gsl>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

namespace logicsim {

class MainWidget : public QWidget {
    Q_OBJECT
   public:
    MainWidget(QWidget* parent = nullptr)
        : QWidget(parent), render_widget_ {new WidgetRenderer(this)} {
        const auto button_widget = build_buttons();

        const auto layout = new QVBoxLayout(this);
        layout->addWidget(button_widget);
        layout->addWidget(render_widget_, 1);

        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        setLayout(layout);

        connect(&timer_, &QTimer::timeout, this, &MainWidget::update_title);

        timer_.setInterval(100);
        timer_.start();

        resize(800, 600);
    }

   private:
    auto build_buttons() -> QWidget* {
        const auto check_box1 = new QCheckBox("&Benchmark");
        const auto check_box2 = new QCheckBox("Render C&ircuit");
        const auto check_box3 = new QCheckBox("Render Co&llision Cache");
        const auto check_box4 = new QCheckBox("Render Co&nnection Cache");

        check_box1->setShortcut(QKeySequence(Qt::ALT | Qt::Key_B));
        check_box2->setShortcut(QKeySequence(Qt::ALT | Qt::Key_I));
        check_box3->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
        check_box4->setShortcut(QKeySequence(Qt::ALT | Qt::Key_N));

        connect(check_box1, &QCheckBox::clicked, this,
                [this](bool enabled) { render_widget_->set_do_benchmark(enabled); });
        connect(check_box2, &QCheckBox::clicked, this,
                [this](bool enabled) { render_widget_->set_do_render_circuit(enabled); });
        connect(check_box3, &QCheckBox::clicked, this, [this](bool enabled) {
            render_widget_->set_do_render_collision_cache(enabled);
        });
        connect(check_box4, &QCheckBox::clicked, this, [this](bool enabled) {
            render_widget_->set_do_render_connection_cache(enabled);
        });

        const auto layout = new QHBoxLayout();
        layout->addWidget(check_box1);
        layout->addWidget(check_box2);
        layout->addWidget(check_box3);
        layout->addWidget(check_box4);
        layout->addStretch(1);

        const auto panel = new QWidget();
        panel->setLayout(layout);

        return panel;
    }

    void update_title() {
        const auto fps = render_widget_->fps();
        const auto scale = render_widget_->scale();
        const auto size = render_widget_->size();

        auto text = fmt::format("[{}x{}] {:.1f} FPS {:.1f} scale", size.width(),
                                size.height(), fps, scale);

        QString title = QString::fromUtf8(text);
        if (title != windowTitle()) {
            setWindowTitle(title);
        }
    }

   private:
    QTimer timer_;

    gsl::not_null<WidgetRenderer*> render_widget_;
};

}  // namespace logicsim
