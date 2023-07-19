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

namespace {

constexpr static int SLIDER_MIN_VALUE = 0;
constexpr static int SLIDER_MAX_VALUE = 600'000;

// auto to_scale = []() {};
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

}  // namespace

auto MainWidget::build_time_rate_slider() -> QWidget* {
    const auto button = new QPushButton("Simulate");
    connect(button, &QPushButton::clicked, this, [this]() {
        render_widget_->set_interaction_state(InteractionState::simulation);
    });
    button->setShortcut(QKeySequence(Qt::Key_F5));

    const auto slider = new QSlider(Qt::Orientation::Horizontal);
    const auto label = new QLabel();

    connect(slider, &QSlider::valueChanged, this,
            [this, label](int value [[maybe_unused]]) {
                const auto rate = from_slider_scale(value);
                render_widget_->set_time_rate(rate);

                label->setText(QString::fromStdString(fmt::format("{}", rate)));
            });

    slider->setMinimum(SLIDER_MIN_VALUE);
    slider->setMaximum(SLIDER_MAX_VALUE);
    slider->setValue(to_slider_scale(time_rate_t {50us}));

    slider->setTickInterval(100'000);
    slider->setTickPosition(QSlider::TickPosition::TicksBothSides);
    label->setMinimumWidth(50);

    const auto layout = new QHBoxLayout();
    layout->addWidget(button);
    layout->addWidget(slider);
    layout->addWidget(label);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

auto MainWidget::element_button(QString label, LogicItemDefinition definition)
    -> QWidget* {
    const auto button = new ElementButton(label);

    connect(button, &QPushButton::clicked, this, [this, definition]() {
        element_definition_ = definition;
        update_selected_logic_item();

        if (element_definition_.element_type == ElementType::wire) {
            render_widget_->set_interaction_state(InteractionState::line_insert);
        } else {
            render_widget_->set_interaction_state(InteractionState::element_insert);
        }
    });

    return button;
}

auto MainWidget::build_element_buttons() -> QWidget* {
    const auto layout = new QGridLayout();
    int row = -1;

    // input count
    const auto input_count = new QSpinBox();
    layout->addWidget(input_count, ++row, 0);

    connect(input_count, &QSpinBox::valueChanged, this, [this](int value) {
        this->input_count_ = value;
        update_selected_logic_item();
    });

    input_count->setValue(gsl::narrow<int>(input_count_));
    input_count->setMinimum(2);
    input_count->setMaximum(connection_id_t::max());

    layout->addWidget(element_button("Wire",
                                     LogicItemDefinition {
                                         .element_type = ElementType::wire,
                                         .input_count = 0,
                                         .orientation = orientation_t::undirected,
                                     }),
                      ++row, 0);

    layout->addWidget(element_button("BTN",
                                     LogicItemDefinition {
                                         .element_type = ElementType::button,
                                         .input_count = 0,
                                         .orientation = orientation_t::undirected,
                                     }),
                      ++row, 0);

    layout->addWidget(element_button("AND",
                                     LogicItemDefinition {
                                         .element_type = ElementType::and_element,
                                         .input_count = 3,
                                         .orientation = orientation_t::right,
                                     }),
                      ++row, 0);
    layout->addWidget(element_button("OR",
                                     LogicItemDefinition {
                                         .element_type = ElementType::or_element,
                                         .input_count = 3,
                                         .orientation = orientation_t::right,
                                     }),
                      ++row, 0);
    layout->addWidget(element_button("XOR",
                                     LogicItemDefinition {
                                         .element_type = ElementType::xor_element,
                                         .input_count = 3,
                                         .orientation = orientation_t::right,
                                     }),
                      ++row, 0);

    layout->addWidget(element_button("NAND",
                                     LogicItemDefinition {
                                         .element_type = ElementType::and_element,
                                         .input_count = 3,
                                         .orientation = orientation_t::right,
                                     }),
                      ++row, 0);
    layout->addWidget(element_button("NOR",
                                     LogicItemDefinition {
                                         .element_type = ElementType::or_element,
                                         .input_count = 3,
                                         .orientation = orientation_t::right,
                                     }),
                      ++row, 0);
    layout->addWidget(element_button("INV",
                                     LogicItemDefinition {
                                         .element_type = ElementType::inverter_element,
                                         .input_count = 1,
                                         .orientation = orientation_t::right,
                                     }),
                      ++row, 0);

    // layout->addWidget(element_button("JK-FF",
    //                                  LogicItemDefinition {
    //                                      .element_type = ElementType::flipflop_jk,
    //                                      .input_count = 5,
    //                                      .output_count = 2,
    //                                      .orientation = orientation_t::right,
    //                                  }),
    //                   ++row, 0);
    // layout->addWidget(element_button("CLK",
    //                                  LogicItemDefinition {
    //                                      .element_type = ElementType::clock_generator,
    //                                      .input_count = 2,
    //                                      .output_count = 2,
    //                                      .orientation = orientation_t::right,
    //                                  }),
    //                   ++row, 0);
    // layout->addWidget(element_button("REG",
    //                                  LogicItemDefinition {
    //                                      .element_type = ElementType::shift_register,
    //                                      .input_count = 3,
    //                                      .output_count = 2,
    //                                      .orientation = orientation_t::right,
    //                                  }),
    //                   ++row, 0);

    layout->setRowStretch(++row, 1);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

auto MainWidget::update_selected_logic_item() -> void {
    auto definition = element_definition_;

    if (definition.element_type == ElementType::and_element
        || definition.element_type == ElementType::or_element
        || definition.element_type == ElementType::xor_element) {
        definition.input_count = input_count_;
    }

    render_widget_->set_element_definition(definition);
}

void MainWidget::update_title() {
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

}  // namespace logicsim
