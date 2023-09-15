#include "main_widget.h"

#include "render_widget.h"
#include "resource.h"
#include "serialize.h"

#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <locale>
#include <ranges>

namespace logicsim {

ElementButton::ElementButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {}

auto ElementButton::sizeHint() const -> QSize {
    const auto text = "NAND";
    const auto margin = 10;

    const auto metric = fontMetrics();
    const auto size = metric.size(Qt::TextShowMnemonic, text);
    const auto extend = std::max(size.height(), size.width()) + margin;

    return QSize(extend, extend);
}

auto ElementButton::minimumSizeHint() const -> QSize {
    return sizeHint();
}

//
// MainWidget
//

MainWidget::MainWidget(QWidget* parent)
    : QMainWindow(parent),
      render_widget_ {new RendererWidget(this)},
      last_saved_data_ {render_widget_->serialize_circuit()} {
    setAcceptDrops(true);

    create_menu();
    create_toolbar();
    // create_statusbar();

    const auto layout = new QVBoxLayout();
    // layout->addWidget(build_delay_slider());
    // layout->addWidget(build_time_rate_slider());

    const auto hlayout = new QHBoxLayout();
    layout->addLayout(hlayout, 1);
    hlayout->addWidget(build_element_buttons(), 0);
    hlayout->addWidget(render_widget_, 1);

    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const auto frame = new QWidget(this);
    frame->setLayout(layout);
    this->setCentralWidget(frame);

    connect(&timer_, &QTimer::timeout, this, &MainWidget::update_title);
    timer_.setInterval(100);
    timer_.start();

    connect(&timer_process_arguments_, &QTimer::timeout, this,
            &MainWidget::process_arguments);
    timer_process_arguments_.setInterval(0);
    timer_process_arguments_.setSingleShot(true);
    timer_process_arguments_.start();

    connect(render_widget_, &RendererWidgetBase::interaction_state_changed, this,
            &MainWidget::on_interaction_state_changed);

    new_circuit();
    resize(914, 700);
}

namespace {

template <typename Callback>
concept action_callable = std::invocable<Callback> || std::invocable<Callback, bool>;

struct ActionAttributes {
    std::optional<QKeySequence> shortcut;
    bool shortcut_auto_repeat {false};
    std::optional<icon_t> icon;
};

auto add_action_impl(QMenu* menu, const QString& text, ActionAttributes attributes,
                     action_callable auto callable) -> QAction* {
    auto* action = menu->addAction(text);
    auto* widget = menu->parentWidget();

    if constexpr (std::invocable<decltype(callable), bool>) {
        widget->connect(action, &QAction::toggled, widget, callable);
    } else {
        widget->connect(action, &QAction::triggered, widget, callable);
    }

    if (attributes.shortcut) {
        action->setShortcut(*attributes.shortcut);
        action->setAutoRepeat(attributes.shortcut_auto_repeat);
    }
    if (attributes.icon) {
        action->setIcon(QIcon(get_icon_path(*attributes.icon)));
    }

    return action;
}

auto add_action(QMenu* menu, const QString& text, ActionAttributes attributes,
                std::invocable<> auto callable) {
    return add_action_impl(menu, text, attributes, callable);
}

struct CheckableAttributes {
    bool start_state {false};
};

auto add_action_checkable(QMenu* menu, const QString& text,
                          ActionAttributes action_attributes,
                          CheckableAttributes checkable_attributes,
                          std::invocable<bool> auto callable) -> QAction* {
    auto* action = add_action_impl(menu, text, action_attributes, callable);
    action->setCheckable(true);

    action->setChecked(checkable_attributes.start_state);
    std::invoke(callable, checkable_attributes.start_state);

    return action;
}

struct GroupAttributes {
    bool active {false};
    QActionGroup* group {nullptr};
};

auto add_action_group(QMenu* menu, const QString& text,
                      ActionAttributes action_attributes,
                      GroupAttributes group_attributes, std::invocable<> auto callable)
    -> QAction* {
    auto* action = add_action_impl(menu, text, action_attributes, callable);
    action->setCheckable(true);

    if (group_attributes.group) {
        action->setActionGroup(group_attributes.group);
    }

    action->setChecked(group_attributes.active);
    if (group_attributes.active) {
        std::invoke(callable);
    }

    return action;
}

}  // namespace

auto MainWidget::create_menu() -> void {
    {
        // File
        auto* menu = menuBar()->addMenu(tr("&File"));

        actions_.new_file = add_action(
            menu, tr("&New"),
            ActionAttributes {.shortcut = QKeySequence::New, .icon = icon_t::new_file},
            [this] { new_circuit(); });
        actions_.open_file = add_action(
            menu, tr("&Open..."),
            ActionAttributes {.shortcut = QKeySequence::Open, .icon = icon_t::open_file},
            [this] { open_circuit(); });
        actions_.save_file = add_action(
            menu, tr("&Save"),
            ActionAttributes {.shortcut = QKeySequence::Save, .icon = icon_t::save_file},
            [this] { save_circuit(filename_choice_t::same_as_last); });
        add_action(menu, tr("Save &As..."),
                   ActionAttributes {.shortcut = QKeySequence::SaveAs},
                   [this] { save_circuit(filename_choice_t::ask_new); });

        menu->addSeparator();
        add_action(
            menu, tr("E&xit"),
            ActionAttributes {.shortcut = QKeySequence::Quit, .icon = icon_t::exit},
            [this]() { close(); });
    }

    {
        // Edit
        auto* menu = menuBar()->addMenu(tr("&Edit"));

        actions_.cut = add_action(
            menu, tr("Cu&t"),
            ActionAttributes {.shortcut = QKeySequence::Cut, .icon = icon_t::cut},
            [this] { render_widget_->cut_selected_items(); });
        actions_.copy = add_action(
            menu, tr("&Copy"),
            ActionAttributes {.shortcut = QKeySequence::Copy, .icon = icon_t::copy},
            [this] { render_widget_->copy_selected_items(); });
        actions_.paste = add_action(
            menu, tr("&Paste"),
            ActionAttributes {.shortcut = QKeySequence::Paste, .icon = icon_t::paste},
            [this] { render_widget_->paste_clipboard_items(); });
        add_action(menu, tr("&Delete"),
                   ActionAttributes {.shortcut = QKeySequence::Delete,
                                     .icon = icon_t::delete_selected},
                   [this] { render_widget_->delete_selected_items(); });
        add_action(menu, tr("Select &All"),
                   ActionAttributes {.shortcut = QKeySequence::SelectAll,
                                     .icon = icon_t::select_all},
                   [this] { render_widget_->select_all_items(); });
    }

    {
        // View
        auto* menu = menuBar()->addMenu(tr("&View"));

        add_action(menu, tr("Zoom &In"),
                   ActionAttributes {.shortcut = QKeySequence::ZoomIn,
                                     .shortcut_auto_repeat = true,
                                     .icon = icon_t::zoom_in},
                   [this] { render_widget_->zoom(+1); });
        add_action(menu, tr("Zoom &Out"),
                   ActionAttributes {.shortcut = QKeySequence::ZoomOut,
                                     .shortcut_auto_repeat = true,
                                     .icon = icon_t::zoom_out},
                   [this] { render_widget_->zoom(-1); });
        add_action(menu, tr("&Reset Zoom"), ActionAttributes {.icon = icon_t::reset_zoom},
                   [this] { render_widget_->reset_view_config(); });
    }

    {
        // Simulation
        auto* menu = menuBar()->addMenu(tr("&Simulation"));

        // Benchmark
        actions_.simulation_start = add_action(
            menu, tr("Start &Simulation"),
            ActionAttributes {.icon = icon_t::simulation_start}, [this]() {
                render_widget_->set_interaction_state(InteractionState::simulation);
            });

        actions_.simulation_stop = add_action(
            menu, tr("Stop &Simulation"),
            ActionAttributes {.icon = icon_t::simulation_stop}, [this]() {
                using enum InteractionState;
                if (render_widget_->interaction_state() == InteractionState::simulation) {
                    render_widget_->set_interaction_state(InteractionState::selection);
                };
            });

        menu->addSeparator();
        actions_.wire_delay = add_action_checkable(
            menu, tr("Wire &Delay"), ActionAttributes {},
            CheckableAttributes {.start_state = true}, [this](bool checked) {
                const auto delay = checked ? Schematic::defaults::wire_delay_per_distance
                                           : delay_t {0ns};
                render_widget_->set_wire_delay_per_distance(delay);
            });

        const auto tooltip_fmt =
            tr("When enabled wires have visible delay of {}/unit.\n"
               "Wire delay can be very useful when understanding circuits.\n"
               "One the other hand it can be a hindrance when designing large\n"
               "sequential circuits.")
                .toStdString();
        const auto tooltip = fmt::format(fmt::runtime(tooltip_fmt),
                                         Schematic::defaults::wire_delay_per_distance);

        actions_.wire_delay->setToolTip(QString::fromStdString(tooltip));
    }

    {
        // Debug
        auto* menu = menuBar()->addMenu(tr("&Debug"));

        // Benchmark
        add_action_checkable(
            menu, tr("&Benchmark"), ActionAttributes {.icon = icon_t::benchmark},
            CheckableAttributes {.start_state = false},
            [this](bool checked) { render_widget_->set_do_benchmark(checked); });

        menu->addSeparator();
        {
            add_action_checkable(
                menu, tr("Show C&ircuit"),
                ActionAttributes {.icon = icon_t::show_circuit},
                CheckableAttributes {.start_state = true},
                [this](bool checked) { render_widget_->set_do_render_circuit(checked); });
            add_action_checkable(
                menu, tr("Show C&ollision Cache"),
                ActionAttributes {.icon = icon_t::show_collision_cache},
                CheckableAttributes {.start_state = false}, [this](bool checked) {
                    render_widget_->set_do_render_collision_cache(checked);
                });
            add_action_checkable(
                menu, tr("Show Co&nnection Cache"),
                ActionAttributes {.icon = icon_t::show_connection_cache},
                CheckableAttributes {.start_state = false}, [this](bool checked) {
                    render_widget_->set_do_render_connection_cache(checked);
                });
            add_action_checkable(
                menu, tr("Show &Selection Cache"),
                ActionAttributes {.icon = icon_t::show_selection_cache},
                CheckableAttributes {.start_state = false}, [this](bool checked) {
                    render_widget_->set_do_render_selection_cache(checked);
                });
        }

        // Examples
        menu->addSeparator();
        add_action(menu, tr("&Reload"), ActionAttributes {.icon = icon_t::reload_circuit},
                   [this]() { render_widget_->reload_circuit(); });
        {
            add_action(menu, tr("Load \"Si&mple\" Example"),
                       ActionAttributes {.icon = icon_t::load_simple_example},
                       [this]() { render_widget_->load_circuit_example(1); });

            add_action(menu, tr("Load \"&Wires\" Example"),
                       ActionAttributes {.icon = icon_t::load_wire_example},
                       [this]() { render_widget_->load_circuit_example(4); });

            add_action(menu, tr("Load \"&Elements\" Example"),
                       ActionAttributes {.icon = icon_t::load_element_example},
                       [this]() { render_widget_->load_circuit_example(3); });

            add_action(menu, tr("Load \"Elements + Wi&res\" Example"),
                       ActionAttributes {.icon = icon_t::load_elements_and_wires_example},
                       [this]() { render_widget_->load_circuit_example(2); });
        }

        // Thread Count
        menu->addSeparator();
        add_action_checkable(
            menu, tr("&Direct Rendering"),
            ActionAttributes {.icon = icon_t::direct_rendering},
            CheckableAttributes {.start_state = true},
            [this](bool checked) { render_widget_->set_use_backing_store(checked); });

        menu->addSeparator();
        {
            auto* group = new QActionGroup(menu);
            add_action_group(menu, tr("S&ynchronous Rendering"), ActionAttributes {},
                             GroupAttributes {.active = false, .group = group},
                             [this]() { render_widget_->set_thread_count(0); });
            add_action_group(menu, tr("&2 Render Threads"), ActionAttributes {},
                             GroupAttributes {.active = false, .group = group},
                             [this]() { render_widget_->set_thread_count(2); });
            add_action_group(menu, tr("&4 Render Threads"), ActionAttributes {},
                             GroupAttributes {.active = true, .group = group},
                             [this]() { render_widget_->set_thread_count(4); });
            add_action_group(menu, tr("&8 Render Threads"), ActionAttributes {},
                             GroupAttributes {.active = false, .group = group},
                             [this]() { render_widget_->set_thread_count(8); });
        }
    }
    {
        // Tools
        auto* menu = menuBar()->addMenu(tr("&Tools"));
        add_action(menu, tr("&Options..."),
                   ActionAttributes {.shortcut = QKeySequence::Preferences,
                                     .icon = icon_t::options},
                   [] { print("options"); });
    }
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
    const auto value_log =
        std::log10(rate.rate_per_second.value.count() / 1000.0) * 100'000;
    return std::clamp(gsl::narrow<int>(std::round(value_log)), SLIDER_MIN_VALUE,
                      SLIDER_MAX_VALUE);
};

}  // namespace detail::time_slider

auto MainWidget::create_toolbar() -> void {
    const auto icon_size = QSize {18, 18};

    // Standard
    {
        auto* toolbar = this->addToolBar("Standard");
        toolbar->setIconSize(icon_size);

        // file actions
        toolbar->addAction(actions_.new_file);
        toolbar->addAction(actions_.open_file);
        toolbar->addAction(actions_.save_file);
        toolbar->addSeparator();

        // edit actions
        toolbar->addAction(actions_.cut);
        toolbar->addAction(actions_.copy);
        toolbar->addAction(actions_.paste);
        toolbar->addSeparator();
    }
    {
        auto* toolbar = this->addToolBar("Simulation");
        toolbar->setIconSize(icon_size);

        // start simulation
        {
            auto* button = new QToolButton(this);
            button->setDefaultAction(actions_.simulation_start);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

            toolbar->addWidget(button);
            toolbar->addSeparator();
        }
        // stop simulation
        {
            auto* button = new QToolButton(this);
            button->setDefaultAction(actions_.simulation_stop);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

            toolbar->addWidget(button);
            toolbar->addSeparator();
        }

        // wire delay
        {
            auto* action = actions_.wire_delay;

            auto* check_box = new QCheckBox(tr("Wire Delay"), this);
            check_box->setChecked(action->isChecked());
            check_box->setToolTip(action->toolTip());

            toolbar->addWidget(check_box);
            toolbar->addSeparator();
            actions_.wire_delay_checkbox = check_box;

            connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
                const auto checked = state == Qt::Checked;
                actions_.wire_delay->setChecked(checked);
            });
            connect(action, &QAction::toggled, this,
                    [check_box](bool checked) { check_box->setChecked(checked); });
        }
    }

    this->addToolBarBreak();

    {
        auto* toolbar = this->addToolBar("Speed");
        toolbar->setIconSize(icon_size);

        {
            using namespace detail::time_slider;

            auto* slider = new QSlider(Qt::Orientation::Horizontal);
            auto* label = new QLabel();

            connect(slider, &QSlider::valueChanged, this, [this, label](int value) {
                const auto rate = from_slider_scale(value);
                render_widget_->set_simulation_time_rate(rate);

                label->setText(QString::fromStdString(fmt::format("{}", rate)));
            });

            slider->setMinimum(SLIDER_MIN_VALUE);
            slider->setMaximum(SLIDER_MAX_VALUE);
            slider->setValue(to_slider_scale(SLIDER_START_VALUE));

            slider->setTickInterval(100'000);
            slider->setTickPosition(QSlider::TickPosition::TicksBothSides);
            label->setMinimumWidth(70);

            toolbar->addWidget(slider);
            toolbar->addWidget(label);
        }
    }
}

auto MainWidget::create_statusbar() -> void {
    auto* statusbar = new QStatusBar(this);

    {
        auto* slider = new QSlider(Qt::Orientation::Horizontal, this);
        slider->setTickPosition(QSlider::TickPosition::TicksBothSides);
        statusbar->addPermanentWidget(slider);
    }

    this->setStatusBar(statusbar);
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

    const auto checkbox = new QCheckBox("Zero");
    const auto slider = new QSlider(Qt::Orientation::Horizontal);
    const auto label = new QLabel();

    connect(slider, &QSlider::valueChanged, this, [&, label](int value) {
        const auto delay = from_slider_scale(value);

        render_widget_->set_wire_delay_per_distance(delay);
        label->setText(to_text(delay));
    });
    connect(checkbox, &QCheckBox::stateChanged, this, [&, slider, label](int value) {
        bool ignore = value == Qt::Checked;

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
        render_widget_->set_simulation_time_rate(rate);

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

    {
        using enum InteractionState;
        layout->addWidget(element_button("BTN", insert_button), ++row, 0);
        layout->addWidget(element_button("Wire", insert_wire), row, 1);
        layout->addWidget(element_button("LED", insert_led), ++row, 0);
        layout->addWidget(element_button("NUM", insert_display_number), ++row, 0);
        layout->addWidget(element_button("ASCII", insert_display_ascii), row, 1);
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

void MainWidget::update_title() {
    const auto fps = render_widget_->fps();
    const auto eps = render_widget_->simulation_events_per_second();
    const auto scale = render_widget_->pixel_scale();
    const auto size = render_widget_->size_device();

    auto text = fmt::format("[{}x{}] {:.1f} FPS {:.1f} pixel scale", size.width(),
                            size.height(), fps, scale);

    if (eps.has_value()) {
        text = fmt::format("{} {:.3g} EPS", text, round_fast(eps.value()));
    }

    if (!last_saved_filename_.empty()) {
        text = fmt::format("{} - {}", text, last_saved_filename_);
    }

    QString title = QString::fromUtf8(text);
    if (title != windowTitle()) {
        setWindowTitle(title);
    }
}

void MainWidget::on_interaction_state_changed(InteractionState new_state) {
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

    // simulation active
    bool simulation_active = new_state == InteractionState::simulation;

    if (actions_.simulation_start) {
        actions_.simulation_start->setEnabled(!simulation_active);
    }
    if (actions_.simulation_stop) {
        actions_.simulation_stop->setEnabled(simulation_active);
    }
    if (actions_.wire_delay) {
        actions_.wire_delay->setEnabled(!simulation_active);
    }
    if (actions_.wire_delay_checkbox) {
        actions_.wire_delay_checkbox->setEnabled(!simulation_active);
    }
}

auto MainWidget::process_arguments() -> void {
    for (const auto& argument_qt : QCoreApplication::arguments() | std::views::drop(1)) {
        const auto argument = argument_qt.toStdString();
        if (QFileInfo(argument_qt).isFile()) {
            open_circuit(argument);
            break;
        }
    }
}

auto MainWidget::filename_filter() const -> QString {
    return tr("Circuit Files (*.ls2)");
}

auto MainWidget::new_circuit() -> void {
    if (ensure_circuit_saved() == save_result_t::success) {
        render_widget_->reset_circuit();
        render_widget_->set_interaction_state(InteractionState::selection);

        last_saved_filename_.clear();
        last_saved_data_ = render_widget_->serialize_circuit();
    }
}

auto MainWidget::save_circuit(filename_choice_t filename_choice) -> save_result_t {
    const auto filename = [&] {
        if (!last_saved_filename_.empty() &&
            filename_choice == filename_choice_t::same_as_last) {
            return last_saved_filename_;
        }
        return QFileDialog::getSaveFileName(this,              //
                                            tr("Save As"),     //
                                            "",                //
                                            filename_filter()  //
                                            )
            .toStdString();
    }();
    if (filename.empty()) {
        return save_result_t::canceled;
    }

    const auto _ [[maybe_unused]] = Timer("Save");

    if (!render_widget_->save_circuit(filename)) {
        const auto message = fmt::format("Failed to save \"{}\".", filename);
        QMessageBox::warning(this,                         //
                             QString::fromUtf8(app_name),  //
                             QString::fromUtf8(message)    //
        );
        return save_circuit(filename_choice_t::ask_new);
    }

    last_saved_filename_ = filename;
    last_saved_data_ = render_widget_->serialize_circuit();

    return save_result_t::success;
}

auto MainWidget::open_circuit(std::optional<std::string> filename) -> void {
    if (ensure_circuit_saved() != save_result_t::success) {
        return;
    }

    if (!filename) {
        filename = QFileDialog::getOpenFileName(this,              //
                                                tr("Open"),        //
                                                "",                //
                                                filename_filter()  //
                                                )
                       .toStdString();
    }

    if (!filename || filename->empty()) {
        return;
    }

    const auto _ [[maybe_unused]] = Timer("Open");

    if (!render_widget_->load_circuit(*filename)) {
        const auto message = fmt::format("Failed to load \"{}\".", filename);
        QMessageBox::warning(this,                         //
                             QString::fromUtf8(app_name),  //
                             QString::fromUtf8(message)    //
        );
    }
    last_saved_filename_ = *filename;
    last_saved_data_ = render_widget_->serialize_circuit();
}

auto MainWidget::ensure_circuit_saved() -> save_result_t {
    if (last_saved_data_ == render_widget_->serialize_circuit()) {
        return save_result_t::success;
    }

    const auto name =
        last_saved_filename_.empty() ? std::string("New Circuit") : last_saved_filename_;
    const auto message = fmt::format("Save file \"{}\"?", name);
    const auto result = QMessageBox::question(
        this,                                                      //
        QString::fromUtf8(app_name),                               //
        QString::fromUtf8(message),                                //
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,  //
        QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        return save_circuit(filename_choice_t::same_as_last);
    }

    else if (result == QMessageBox::No) {
        return save_result_t::success;
    }

    return save_result_t::canceled;
}

auto MainWidget::closeEvent(QCloseEvent* event) -> void {
    event->ignore();

    if (ensure_circuit_saved() == save_result_t::success) {
        event->accept();
    }
}

auto MainWidget::dragEnterEvent(QDragEnterEvent* event) -> void {
    const auto& mimeData = *event->mimeData();

    if (mimeData.hasUrls() && mimeData.urls().size() == 1 &&
        mimeData.urls().front().isLocalFile()) {
        event->acceptProposedAction();
    }
}

auto MainWidget::dropEvent(QDropEvent* event) -> void {
    const auto& mimeData = *event->mimeData();

    if (mimeData.hasUrls() && mimeData.urls().size() == 1 &&
        mimeData.urls().front().isLocalFile()) {
        const auto filename = mimeData.urls().front().toLocalFile().toStdString();
        open_circuit(filename);
    }
}

}  // namespace logicsim
