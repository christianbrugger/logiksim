#include "main_widget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QVBoxLayout>

namespace logicsim {

MainWidget::MainWidget(QWidget* parent)
    : QWidget(parent), render_widget_ {new RendererWidget(this)} {
    const auto render_panel = build_render_buttons();
    const auto mode_panel = build_mode_buttons();

    const auto layout = new QVBoxLayout(this);
    layout->addWidget(render_panel);
    layout->addWidget(mode_panel);
    layout->addWidget(render_widget_, 1);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(&timer_, &QTimer::timeout, this, &MainWidget::update_title);

    timer_.setInterval(100);
    timer_.start();

    resize(800, 600);
}

auto MainWidget::build_render_buttons() -> QWidget* {
    const auto check_box1 = new QCheckBox("&Benchmark");
    const auto check_box2 = new QCheckBox("Render C&ircuit");
    const auto check_box3 = new QCheckBox("Render Co&llision Cache");
    const auto check_box4 = new QCheckBox("Render Co&nnection Cache");
    const auto check_box5 = new QCheckBox("Render S&election Cache");

    check_box1->setShortcut(QKeySequence(Qt::ALT | Qt::Key_B));
    check_box2->setShortcut(QKeySequence(Qt::ALT | Qt::Key_I));
    check_box3->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
    check_box4->setShortcut(QKeySequence(Qt::ALT | Qt::Key_N));
    check_box5->setShortcut(QKeySequence(Qt::ALT | Qt::Key_E));

    connect(check_box1, &QCheckBox::stateChanged, this,
            [this](bool enabled) { render_widget_->set_do_benchmark(enabled); });
    connect(check_box2, &QCheckBox::stateChanged, this,
            [this](bool enabled) { render_widget_->set_do_render_circuit(enabled); });
    connect(check_box3, &QCheckBox::stateChanged, this, [this](bool enabled) {
        render_widget_->set_do_render_collision_cache(enabled);
    });
    connect(check_box4, &QCheckBox::stateChanged, this, [this](bool enabled) {
        render_widget_->set_do_render_connection_cache(enabled);
    });
    connect(check_box5, &QCheckBox::stateChanged, this, [this](bool enabled) {
        render_widget_->set_do_render_selection_cache(enabled);
    });

    // startup states
    check_box2->setCheckState(Qt::Checked);

    const auto layout = new QHBoxLayout();
    layout->addWidget(check_box1);
    layout->addWidget(check_box2);
    layout->addWidget(check_box3);
    layout->addWidget(check_box4);
    layout->addWidget(check_box5);
    layout->addStretch(1);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

auto MainWidget::build_mode_buttons() -> QWidget* {
    const auto radio1 = new QRadioButton("&Select");
    const auto radio2 = new QRadioButton("Inse&rt");

    radio1->setShortcut(QKeySequence(Qt::ALT | Qt::Key_S));
    radio2->setShortcut(QKeySequence(Qt::ALT | Qt::Key_R));

    connect(radio1, &QRadioButton::toggled, this, [this](bool enabled) {
        if (enabled) {
            render_widget_->set_interaction_state(InteractionState::select);
        }
    });
    connect(radio2, &QRadioButton::toggled, this, [this](bool enabled) {
        if (enabled) {
            render_widget_->set_interaction_state(InteractionState::insert);
        }
    });

    // startup states
    radio1->setChecked(true);

    const auto layout = new QHBoxLayout();
    layout->addWidget(radio1);
    layout->addWidget(radio2);
    layout->addStretch(1);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

void MainWidget::update_title() {
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

}  // namespace logicsim
