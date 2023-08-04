#include "main_widget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace logicsim {
class ElementButton : public QPushButton {
    // TODO use Q_OBJECT
    // Q_OBJECT
   public:
    explicit ElementButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent) {}

    auto sizeHint() const -> QSize override {
        const auto text = "NAND";
        const auto margin = 10;

        const auto metric = fontMetrics();
        const auto size = metric.size(Qt::TextShowMnemonic, text);
        const auto extend = std::max(size.height(), size.width()) + margin;

        return QSize(extend, extend);
    }

    QSize minimumSizeHint() const override {
        return sizeHint();
    }
};

//
// MainWidget
//

MainWidget::MainWidget(QWidget* parent)
    : QWidget(parent), render_widget_ {new RendererWidget(this)} {
    const auto layout = new QVBoxLayout(this);
    layout->addWidget(build_render_buttons());
    layout->addWidget(build_mode_buttons());
    layout->addWidget(build_delay_slider());
    layout->addWidget(build_time_rate_slider());

    const auto hlayout = new QHBoxLayout();
    layout->addLayout(hlayout, 1);
    hlayout->addWidget(build_element_buttons(), 0);
    hlayout->addWidget(render_widget_, 1);

    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(&timer_, &QTimer::timeout, this, &MainWidget::update_title);

    timer_.setInterval(100);
    timer_.start();

    connect(render_widget_, &RendererWidget::interaction_state_changed, this,
            &MainWidget::on_interaction_state_changed);

    render_widget_->set_interaction_state(InteractionState::selection);
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
    const auto button0 = new QPushButton("Reload");
    const auto button1 = new QPushButton("Simple");
    const auto button2 = new QPushButton("Elements + Wires");
    const auto button3 = new QPushButton("Elements");
    const auto button4 = new QPushButton("Wires");

    connect(button0, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { render_widget_->reload_circuit(); });
    connect(button1, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { render_widget_->load_circuit(1); });
    connect(button2, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { render_widget_->load_circuit(2); });
    connect(button3, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { render_widget_->load_circuit(3); });
    connect(button4, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { render_widget_->load_circuit(4); });

    const auto layout = new QHBoxLayout();
    layout->addWidget(button0);
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);
    layout->addStretch(1);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

namespace detail::delay_slider {

constexpr static int SLIDER_MIN_VALUE = 0;
constexpr static int SLIDER_MAX_VALUE = 400'000;
constexpr static auto SLIDER_START_VALUE = Schematic::defaults::wire_delay_per_distance;

auto from_slider_scale(int value) -> delay_t {
    const double value_ns = std::pow(10.0, value / double {100'000.0});
    return delay_t {1ns * gsl::narrow<int64_t>(std::round(value_ns))};
};

auto to_slider_scale(delay_t delay) -> int {
    const auto value_log = std::log10(delay.value.count()) * 100'000;
    return std::clamp(gsl::narrow<int>(std::round(value_log)), SLIDER_MIN_VALUE,
                      SLIDER_MAX_VALUE);
};

auto to_text(delay_t delay) -> QString {
    if (delay > delay_t {0ns}) {
        return QString::fromStdString(fmt::format("{}/unit", delay));
    }
    return QString::fromStdString("1ns flat");
}

}  // namespace detail::delay_slider

auto MainWidget::build_delay_slider() -> QWidget* {
    using namespace detail::delay_slider;

    const auto checkbox = new QCheckBox("Ignore");
    const auto slider = new QSlider(Qt::Orientation::Horizontal);
    const auto label = new QLabel();

    connect(slider, &QSlider::valueChanged, this, [&, label](int value) {
        const auto delay = from_slider_scale(value);

        render_widget_->set_wire_delay_per_distance(delay);
        label->setText(to_text(delay));
    });
    connect(checkbox, &QCheckBox::stateChanged, this, [&, slider, label](int ignore) {
        slider->setEnabled(!ignore);
        const auto delay = ignore ? delay_t {0ns} : from_slider_scale(slider->value());

        render_widget_->set_wire_delay_per_distance(delay);
        label->setText(to_text(delay));
    });

    slider->setMinimum(SLIDER_MIN_VALUE);
    slider->setMaximum(SLIDER_MAX_VALUE);
    slider->setValue(to_slider_scale(SLIDER_START_VALUE));

    slider->setTickInterval(100'000);
    slider->setTickPosition(QSlider::TickPosition::TicksBothSides);
    label->setMinimumWidth(70);

    const auto layout = new QHBoxLayout();
    layout->addWidget(checkbox);
    layout->addWidget(slider);
    layout->addWidget(label);

    const auto panel = new QWidget();
    panel->setLayout(layout);

    delay_slider_ = slider;
    delay_panel_ = panel;

    return panel;
}

namespace detail::time_slider {

constexpr static int SLIDER_MIN_VALUE = 0;
constexpr static int SLIDER_MAX_VALUE = 700'000;
constexpr static auto SLIDER_START_VALUE = time_rate_t {2ms};

auto from_slider_scale(int value) -> time_rate_t {
    if (value == SLIDER_MIN_VALUE) {
        return time_rate_t {0us};
    }

    const double value_ns = std::pow(10.0, value / double {100'000.0}) * 1000.0;
    return time_rate_t {1ns * gsl::narrow<int64_t>(std::round(value_ns))};
};

auto to_slider_scale(time_rate_t rate) -> int {
    const auto value_log
        = std::log10(rate.rate_per_second.value.count() / 1000.0) * 100'000;
    return std::clamp(gsl::narrow<int>(std::round(value_log)), SLIDER_MIN_VALUE,
                      SLIDER_MAX_VALUE);
};

}  // namespace detail::time_slider

auto MainWidget::build_time_rate_slider() -> QWidget* {
    using namespace detail::time_slider;

    const auto button = new QPushButton("Simulate");
    connect(button, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            render_widget_->set_interaction_state(InteractionState::simulation);
        } else {
            render_widget_->set_interaction_state(InteractionState::selection);
        }
    });
    button->setShortcut(QKeySequence(Qt::Key_F5));
    button->setCheckable(true);
    button_map_[InteractionState::simulation] = button;

    const auto slider = new QSlider(Qt::Orientation::Horizontal);
    const auto label = new QLabel();

    connect(slider, &QSlider::valueChanged, this, [this, label](int value) {
        const auto rate = from_slider_scale(value);
        render_widget_->set_time_rate(rate);

        label->setText(QString::fromStdString(fmt::format("{}", rate)));
    });

    slider->setMinimum(SLIDER_MIN_VALUE);
    slider->setMaximum(SLIDER_MAX_VALUE);
    slider->setValue(to_slider_scale(SLIDER_START_VALUE));

    slider->setTickInterval(100'000);
    slider->setTickPosition(QSlider::TickPosition::TicksBothSides);
    label->setMinimumWidth(70);

    const auto layout = new QHBoxLayout();
    layout->addWidget(button);
    layout->addWidget(slider);
    layout->addWidget(label);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

auto MainWidget::element_button(QString label, InteractionState state) -> QWidget* {
    const auto button = new ElementButton(label);
    button->setCheckable(true);
    button_map_[state] = button;

    connect(button, &QPushButton::clicked, this,
            [this, state]() { render_widget_->set_interaction_state(state); });

    return button;
}

auto line_separator() -> QWidget* {
    const auto line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

auto MainWidget::build_element_buttons() -> QWidget* {
    const auto layout = new QGridLayout();
    int row = -1;

    // input count
    const auto input_count = new QSpinBox();
    layout->addWidget(input_count, ++row, 0, 1, 2);
    layout->addWidget(line_separator(), ++row, 0, 1, 2);

    connect(input_count, &QSpinBox::valueChanged, this,
            [this](int value) { render_widget_->set_default_input_count(value); });

    input_count->setValue(gsl::narrow<int>(render_widget_->default_input_count()));
    input_count->setMinimum(2);
    input_count->setMaximum(connection_id_t::max());

    {
        using enum InteractionState;
        layout->addWidget(element_button("BTN", insert_button), ++row, 0);
        layout->addWidget(element_button("Wire", insert_wire), row, 1);
        layout->addWidget(line_separator(), ++row, 0, 1, 2);

        layout->addWidget(element_button("AND", insert_and_element), ++row, 0);
        layout->addWidget(element_button("NAND", insert_nand_element), row, 1);
        layout->addWidget(element_button("OR", insert_or_element), ++row, 0);
        layout->addWidget(element_button("NOR", insert_nor_element), row, 1);
        layout->addWidget(element_button("BUF", insert_buffer_element), ++row, 0);
        layout->addWidget(element_button("INV", insert_inverter_element), row, 1);
        layout->addWidget(element_button("XOR", insert_xor_element), ++row, 0);
        layout->addWidget(line_separator(), ++row, 0, 1, 2);

        layout->addWidget(element_button("Latch", insert_latch_d), ++row, 0);
        layout->addWidget(element_button("FF", insert_flipflop_d), row, 1);
        layout->addWidget(element_button("MS-FF", insert_flipflop_ms_d), ++row, 0);
        layout->addWidget(element_button("JK-FF", insert_flipflop_jk), row, 1);
        layout->addWidget(line_separator(), ++row, 0, 1, 2);

        layout->addWidget(element_button("CLK", insert_clock_generator), ++row, 0);
        layout->addWidget(element_button("REG", insert_shift_register), row, 1);
    }

    layout->setRowStretch(++row, 1);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

auto MainWidget::update_title() -> void {
    const auto fps = render_widget_->fps();
    const auto scale = render_widget_->pixel_scale();
    const auto size = render_widget_->pixel_size();

    auto text = fmt::format("[{}x{}] {:.1f} FPS {:.1f} pixel scale", size.width(),
                            size.height(), fps, scale);

    QString title = QString::fromUtf8(text);
    if (title != windowTitle()) {
        setWindowTitle(title);
    }
}

auto MainWidget::on_interaction_state_changed(InteractionState new_state) -> void {
    // buttons
    for (auto&& [state, button] : button_map_) {
        if (button != nullptr) {
            button->setChecked(new_state == state);
        }
    }

    // delay slider
    if (delay_panel_ != nullptr) {
        delay_panel_->setEnabled(new_state != InteractionState::simulation);
    }
}

}  // namespace logicsim
