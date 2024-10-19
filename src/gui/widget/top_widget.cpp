#include "gui/widget/top_widget.h"

#include "gui/format/qt_type.h"
#include "gui/qt/path_conversion.h"
#include "gui/qt/setting_location.h"
#include "gui/widget/circuit_widget.h"

#include "core/algorithm/round.h"
#include "core/file.h"
#include "core/format/std_type.h"
#include "core/logging.h"
#include "core/resource.h"
#include "core/serialize.h"
#include "core/serialize_gui_setting.h"
#include "core/timer.h"
#include "core/vocabulary/simulation_config.h"

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
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <ranges>

namespace logicsim {

ElementButton::ElementButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {}

auto ElementButton::sizeHint() const -> QSize {
    const auto text = "NAND";
    const auto margin = 5;

    const auto metric = fontMetrics();
    const auto size = metric.size(Qt::TextShowMnemonic, text);
    const auto extend = std::max(size.height(), size.width()) + margin;

    return QSize {extend, extend};
}

auto ElementButton::minimumSizeHint() const -> QSize {
    return sizeHint();
}

//
// MainWidget
//

TopWidget::TopWidget(QWidget* parent)
    : QMainWindow(parent),
      circuit_widget_ {new CircuitWidget {this}},
      circuit_widget_layout_ {new QHBoxLayout {}},
      last_saved_data_ {circuit_widget_->serialized_circuit()} {
    setWindowIcon(QIcon(to_qt(get_icon_path(icon_t::app_icon))));
    setAcceptDrops(true);

    create_menu();
    create_toolbar();
    // create_statusbar();

    const auto layout = new QVBoxLayout {};

    const auto hlayout = new QHBoxLayout {};
    layout->addLayout(hlayout, 1);
    hlayout->addWidget(build_element_buttons(), 0);
    hlayout->addLayout(circuit_widget_layout_, 1);
    circuit_widget_layout_->addWidget(circuit_widget_, 1);

    circuit_widget_layout_->setContentsMargins(0, 0, 0, 0);
    circuit_widget_layout_->setSpacing(0);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const auto frame = new QWidget {this};
    frame->setLayout(layout);
    this->setCentralWidget(frame);

    // timer title update
    connect(&timer_update_title_, &QTimer::timeout, this,
            &TopWidget::on_timer_update_title);
    timer_update_title_.setInterval(100);
    timer_update_title_.start();

    // timer app arguments
    connect(&timer_process_app_arguments_once_, &QTimer::timeout, this,
            &TopWidget::on_timer_process_app_arguments_once);
    timer_process_app_arguments_once_.setInterval(0);
    timer_process_app_arguments_once_.setSingleShot(true);
    timer_process_app_arguments_once_.start();

    // circuit widget signals
    connect(circuit_widget_, &CircuitWidgetBase::circuit_state_changed, this,
            &TopWidget::on_circuit_state_changed);
    connect(circuit_widget_, &CircuitWidgetBase::simulation_config_changed, this,
            &TopWidget::on_simulation_config_changed);
    connect(circuit_widget_, &CircuitWidgetBase::render_config_changed, this,
            &TopWidget::on_render_config_changed);

    on_circuit_state_changed(circuit_widget_->circuit_state());
    on_simulation_config_changed(circuit_widget_->simulation_config());
    on_render_config_changed(circuit_widget_->render_config());

    new_circuit();
    resize(914, 500);
    restore_gui_state();
}

namespace detail::time_slider {

using namespace std::chrono_literals;

constexpr static int SLIDER_MIN_VALUE = 0;
constexpr static int SLIDER_MIN_NS = 1000;
constexpr static int SLIDER_MAX_VALUE = 700'000;
constexpr static int SLIDER_TICK_INTERVAL = 100'000;

constexpr static auto TIME_RATE_MENU_ITEMS = std::array {
    // anything from 0 - 1us is set to 0, so minimum is 1.001us
    time_rate_t {0ns},   time_rate_t {1001ns}, time_rate_t {10us},
    time_rate_t {100us}, time_rate_t {1ms},    time_rate_t {10ms},
    time_rate_t {100ms}, time_rate_t {1s},     time_rate_t {10s},
};

auto from_slider_scale(int value) -> time_rate_t {
    if (value == SLIDER_MIN_VALUE) {
        return time_rate_t {0us};
    }

    const double value_ns =
        std::pow(10.0, value / double {SLIDER_TICK_INTERVAL}) * double {SLIDER_MIN_NS};
    return time_rate_t {1ns * gsl::narrow<int64_t>(std::round(value_ns))};
};

auto to_slider_scale(time_rate_t rate) -> int {
    if (rate == time_rate_t {0us}) {
        return SLIDER_MIN_VALUE;
    }

    const auto value_log =
        std::log10(gsl::narrow<double>(rate.rate_per_second.count_ns()) /
                   double {SLIDER_MIN_NS}) *
        double {SLIDER_TICK_INTERVAL};
    return std::clamp(gsl::narrow<int>(std::round(value_log)), SLIDER_MIN_VALUE,
                      SLIDER_MAX_VALUE);
};

}  // namespace detail::time_slider

namespace {

template <typename Callback>
concept action_callable = std::invocable<Callback> || std::invocable<Callback, bool>;

struct ActionAttributes {
    std::optional<QKeySequence> shortcut {};
    bool shortcut_auto_repeat {false};
    std::optional<icon_t> icon {};
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
        action->setIcon(QIcon(to_qt(get_icon_path(*attributes.icon))));
    }

    return action;
}

auto add_action(QMenu* menu, const QString& text, ActionAttributes attributes,
                std::invocable<> auto callable) {
    return add_action_impl(menu, text, attributes, callable);
}

auto add_action_checkable(QMenu* menu, const QString& text,
                          ActionAttributes action_attributes,
                          std::invocable<bool> auto callable) -> QAction* {
    auto* action = add_action_impl(menu, text, action_attributes, callable);
    action->setCheckable(true);

    return action;
}

struct GroupAttributes {
    QActionGroup* group {nullptr};
};

auto add_action_group(QMenu* menu, const QString& text,
                      ActionAttributes action_attributes,
                      GroupAttributes group_attributes,
                      std::invocable<> auto callable) -> QAction* {
    auto* action = add_action_impl(menu, text, action_attributes, callable);
    action->setCheckable(true);

    if (group_attributes.group) {
        action->setActionGroup(group_attributes.group);
    }

    return action;
}

}  // namespace

auto TopWidget::create_menu() -> void {
    using UserAction = circuit_widget::UserAction;

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
            [this] { circuit_widget_->do_action(UserAction::cut_selected); });
        actions_.copy = add_action(
            menu, tr("&Copy"),
            ActionAttributes {.shortcut = QKeySequence::Copy, .icon = icon_t::copy},
            [this] { circuit_widget_->do_action(UserAction::copy_selected); });
        actions_.paste = add_action(
            menu, tr("&Paste"),
            ActionAttributes {.shortcut = QKeySequence::Paste, .icon = icon_t::paste},
            [this] { circuit_widget_->do_action(UserAction::paste_from_clipboard); });
        add_action(menu, tr("&Delete"),
                   ActionAttributes {.shortcut = QKeySequence::Delete,
                                     .icon = icon_t::delete_selected},
                   [this] { circuit_widget_->do_action(UserAction::delete_selected); });
        add_action(menu, tr("Select &All"),
                   ActionAttributes {.shortcut = QKeySequence::SelectAll,
                                     .icon = icon_t::select_all},
                   [this] { circuit_widget_->do_action(UserAction::select_all); });
    }

    {
        // View
        auto* menu = menuBar()->addMenu(tr("&View"));

        add_action(menu, tr("Zoom &In"),
                   ActionAttributes {.shortcut = QKeySequence::ZoomIn,
                                     .shortcut_auto_repeat = true,
                                     .icon = icon_t::zoom_in},
                   [this] { circuit_widget_->do_action(UserAction::zoom_in); });
        add_action(menu, tr("Zoom &Out"),
                   ActionAttributes {.shortcut = QKeySequence::ZoomOut,
                                     .shortcut_auto_repeat = true,
                                     .icon = icon_t::zoom_out},
                   [this] { circuit_widget_->do_action(UserAction::zoom_out); });
        add_action(menu, tr("&Reset Zoom"), ActionAttributes {.icon = icon_t::reset_zoom},
                   [this] { circuit_widget_->do_action(UserAction::reset_view); });

        menu->addSeparator();

        menu_toolbars_ = menu->addMenu(tr("&Toolbars"));

        menu->addSeparator();

        {
            auto* submenu = menu->addMenu(tr("&Wire Style"));
            // submenu->setIcon(QIcon(to_qt(get_icon_path(icon_t::simulation_speed))));

            auto* group = new QActionGroup(submenu);

            actions_.wire_render_style_red = add_action_group(
                submenu, tr("&Red"), ActionAttributes {},
                GroupAttributes {.group = group}, [this]() {
                    set_wire_render_style(*circuit_widget_, WireRenderStyle::red);
                });

            actions_.wire_render_style_bold = add_action_group(
                submenu, tr("&Bold"), ActionAttributes {},
                GroupAttributes {.group = group}, [this]() {
                    set_wire_render_style(*circuit_widget_, WireRenderStyle::bold);
                });

            actions_.wire_render_style_bold_red = add_action_group(
                submenu, tr("B&old Red"), ActionAttributes {},
                GroupAttributes {.group = group}, [this]() {
                    set_wire_render_style(*circuit_widget_, WireRenderStyle::bold_red);
                });
        }
    }

    {
        // Simulation
        auto* menu = menuBar()->addMenu(tr("&Simulation"));

        actions_.simulation_start = add_action(
            menu, tr("Start &Simulation"),
            ActionAttributes {.shortcut = QKeySequence {Qt::Key_F5},
                              .icon = icon_t::simulation_start},
            [this]() { circuit_widget_->set_circuit_state(SimulationState {}); });

        actions_.simulation_stop =
            add_action(menu, tr("Stop &Simulation"),
                       ActionAttributes {.shortcut = QKeySequence {Qt::Key_F6},
                                         .icon = icon_t::simulation_stop},
                       [this]() { stop_simulation(*circuit_widget_); });

        menu->addSeparator();
        actions_.wire_delay = add_action_checkable(
            menu, tr("Wire &Delay"), ActionAttributes {}, [this](bool checked) {
                if (circuit_widget_->simulation_config().use_wire_delay != checked) {
                    set_use_wire_delay(*circuit_widget_, checked);
                }
            });

        const auto tooltip_fmt =
            tr("When enabled wires have visible delay of {}/unit.\n"
               "Wire delay can be very useful when understanding circuits.\n"
               "One the other hand it can be a hindrance when designing large\n"
               "sequential circuits.")
                .toStdString();
        const auto tooltip =
            fmt::format(fmt::runtime(tooltip_fmt), default_wire_delay_per_distance());

        actions_.wire_delay->setToolTip(QString::fromStdString(tooltip));

        menu->addSeparator();

        {
            auto* submenu = menu->addMenu(tr("Simulation Speed"));
            submenu->setIcon(QIcon(to_qt(get_icon_path(icon_t::simulation_speed))));

            for (const auto& time_rate : detail::time_slider::TIME_RATE_MENU_ITEMS) {
                const auto text = fmt::format("{}", time_rate);
                add_action(submenu, QString::fromStdString(text), {},
                           [this, time_rate] { set_time_rate_slider(time_rate); });
            }
        }
    }

    {
        // Debug
        auto* menu = menuBar()->addMenu(tr("&Debug"));

        // Benchmark
        actions_.do_benchmark = add_action_checkable(
            menu, tr("&Benchmark"), ActionAttributes {.icon = icon_t::benchmark},

            [this](bool checked) { set_do_benchmark(*circuit_widget_, checked); });

        menu->addSeparator();
        {
            actions_.show_circuit = add_action_checkable(
                menu, tr("Show C&ircuit"),
                ActionAttributes {.icon = icon_t::show_circuit},
                [this](bool checked) { set_show_circuit(*circuit_widget_, checked); });
            actions_.show_collision_cache = add_action_checkable(
                menu, tr("Show C&ollision Cache"),
                ActionAttributes {.icon = icon_t::show_collision_cache},
                [this](bool checked) {
                    set_show_collision_cache(*circuit_widget_, checked);
                });
            actions_.show_connection_cache = add_action_checkable(
                menu, tr("Show Co&nnection Cache"),
                ActionAttributes {.icon = icon_t::show_connection_cache},
                [this](bool checked) {
                    set_show_connection_cache(*circuit_widget_, checked);
                });
            actions_.show_selection_cache = add_action_checkable(
                menu, tr("Show &Selection Cache"),
                ActionAttributes {.icon = icon_t::show_selection_cache},
                [this](bool checked) {
                    set_show_selection_cache(*circuit_widget_, checked);
                });
        }

        // Examples
        menu->addSeparator();
        add_action(menu, tr("&Reload"), ActionAttributes {.icon = icon_t::reload_circuit},
                   [this]() { circuit_widget_->do_action(UserAction::reload_circuit); });
        {
            add_action(menu, tr("Load \"Si&mple\" Example"),
                       ActionAttributes {.icon = icon_t::load_simple_example},
                       [this]() { load_circuit_example(1); });

            add_action(menu, tr("Load \"&Wires\" Example"),
                       ActionAttributes {.icon = icon_t::load_wire_example},
                       [this]() { load_circuit_example(4); });

            add_action(menu, tr("Load \"&Elements\" Example"),
                       ActionAttributes {.icon = icon_t::load_element_example},
                       [this]() { load_circuit_example(3); });

            add_action(menu, tr("Load \"Elements + Wi&res\" Example"),
                       ActionAttributes {.icon = icon_t::load_elements_and_wires_example},
                       [this]() { load_circuit_example(2); });
        }

        menu->addSeparator();

        actions_.show_render_borders = add_action_checkable(
            menu, tr("Show Render Borders"),
            ActionAttributes {.icon = icon_t::show_render_borders},
            [this](bool checked) { set_show_render_borders(*circuit_widget_, checked); });

        actions_.show_mouse_position = add_action_checkable(
            menu, tr("Show Mouse Position"),
            ActionAttributes {.icon = icon_t::show_mouse_position},
            [this](bool checked) { set_show_mouse_position(*circuit_widget_, checked); });

        actions_.non_interactive_mode = add_action_checkable(
            menu, tr("Enter Non-In&teractive Mode"),
            ActionAttributes {.icon = icon_t::non_interactive_mode},
            [this](bool checked) {
                if (checked) {
                    circuit_widget_->set_circuit_state(NonInteractiveState {});
                }
            });

        {
            auto* submenu = menu->addMenu(tr("Content Margins"));
            // submenu->setIcon(QIcon(to_qt(get_icon_path(icon_t::simulation_speed))));

            add_action(submenu, "Add 1 horizontal margin",
                       ActionAttributes {
                           .shortcut = QKeySequence {Qt::CTRL | Qt::SHIFT | Qt::Key_H},
                           .shortcut_auto_repeat = true,
                       },
                       [this] {
                           auto margins = this->circuit_widget_layout_->contentsMargins();
                           margins.setLeft(margins.left() + 1);
                           this->circuit_widget_layout_->setContentsMargins(margins);
                           this->circuit_widget_layout_->update();
                       });
            add_action(submenu, "Add 1 vertical margin",
                       ActionAttributes {
                           .shortcut = QKeySequence {Qt::CTRL | Qt::SHIFT | Qt::Key_V},
                           .shortcut_auto_repeat = true,
                       },
                       [this] {
                           auto margins = this->circuit_widget_layout_->contentsMargins();
                           margins.setTop(margins.top() + 1);
                           this->circuit_widget_layout_->setContentsMargins(margins);
                           this->circuit_widget_layout_->update();
                       });
            add_action(submenu, "Reset content margin",
                       ActionAttributes {
                           .shortcut = QKeySequence {Qt::CTRL | Qt::SHIFT | Qt::Key_R},
                           .shortcut_auto_repeat = true,
                       },
                       [this] {
                           this->circuit_widget_layout_->setContentsMargins(0, 0, 0, 0);
                           this->circuit_widget_layout_->update();
                       });
        }

        menu->addSeparator();
        actions_.direct_rendering = add_action_checkable(
            menu, tr("&Direct Rendering"),
            ActionAttributes {
                .shortcut = QKeySequence {Qt::CTRL | Qt::SHIFT | Qt::Key_B},
                .icon = icon_t::direct_rendering,
            },
            [this](bool checked) { set_direct_rendering(*circuit_widget_, checked); });

        actions_.jit_rendering = add_action_checkable(
            menu, tr("&JIT Rendering"),
            ActionAttributes {
                .shortcut = QKeySequence {Qt::CTRL | Qt::SHIFT | Qt::Key_J},
                .icon = icon_t::jit_rendering,
            },
            [this](bool checked) { set_jit_rendering(*circuit_widget_, checked); });

        // Thread Count
        menu->addSeparator();
        {
            auto* group = new QActionGroup(menu);

            actions_.thread_count_synchronous = add_action_group(
                menu, tr("S&ynchronous Rendering"), ActionAttributes {},
                GroupAttributes {.group = group}, [this]() {
                    set_thread_count(*circuit_widget_, ThreadCount::synchronous);
                });

            actions_.thread_count_two = add_action_group(
                menu, tr("&2 Render Threads"), ActionAttributes {},
                GroupAttributes {.group = group},
                [this]() { set_thread_count(*circuit_widget_, ThreadCount::two); });

            actions_.thread_count_four = add_action_group(
                menu, tr("&4 Render Threads"), ActionAttributes {},
                GroupAttributes {.group = group},
                [this]() { set_thread_count(*circuit_widget_, ThreadCount::four); });

            actions_.thread_count_eight = add_action_group(
                menu, tr("&8 Render Threads"), ActionAttributes {},
                GroupAttributes {.group = group},
                [this]() { set_thread_count(*circuit_widget_, ThreadCount::eight); });
        }
    }

    {
        // Tools
        auto* menu = menuBar()->addMenu(tr("&Tools"));
        menu->menuAction()->setVisible(false);

        add_action(menu, tr("&Options..."),
                   ActionAttributes {.shortcut = QKeySequence::Preferences,
                                     .icon = icon_t::options},
                   [] { print("options"); });
    }
    {
        // About
        auto* menu = menuBar()->addMenu(tr("&Help"));

        add_action(menu, tr("&About"), ActionAttributes {.icon = icon_t::about},
                   [this] { show_about_dialog(); });
    }
}

auto TopWidget::create_toolbar() -> void {
    const auto icon_size = QSize {18, 18};

    // Standard Toolbar
    {
        auto* toolbar = this->addToolBar("Standard");
        toolbar->setObjectName("toolbar_standard");
        toolbar->setIconSize(icon_size);
        menu_toolbars_->addAction(toolbar->toggleViewAction());

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

    // Simulation Toolbar
    {
        auto* toolbar = this->addToolBar("Simulation");
        toolbar->setObjectName("toolbar_simulation");
        toolbar->setIconSize(icon_size);
        menu_toolbars_->addAction(toolbar->toggleViewAction());

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

    // Speed Toolbar
    {
        auto* toolbar = this->addToolBar("Speed");
        toolbar->setObjectName("toolbar_speed");
        toolbar->setIconSize(icon_size);
        menu_toolbars_->addAction(toolbar->toggleViewAction());

        {
            using namespace detail::time_slider;

            auto* slider = new QSlider(Qt::Orientation::Horizontal);
            auto* label = new QLabel();

            connect(slider, &QSlider::valueChanged, this, [this, label](int value) {
                const auto rate = from_slider_scale(value);
                if (rate != circuit_widget_->simulation_config().simulation_time_rate) {
                    set_simulation_time_rate(*circuit_widget_, rate);
                }
                label->setText(QString::fromStdString(fmt::format("{}", rate)));
            });

            slider->setMinimum(SLIDER_MIN_VALUE);
            slider->setMaximum(SLIDER_MAX_VALUE);

            slider->setTickInterval(SLIDER_TICK_INTERVAL);
            slider->setTickPosition(QSlider::TickPosition::TicksBothSides);
            label->setMinimumWidth(70);

            slider->setToolTip(
                tr("Set the speed at which the\n"
                   "simulation is running per second."));

            toolbar->addWidget(slider);
            toolbar->addWidget(label);
            time_rate_slider_ = slider;
        }
    }
}

auto TopWidget::create_statusbar() -> void {
    auto* statusbar = new QStatusBar(this);
    this->setStatusBar(statusbar);
}

auto TopWidget::new_button(const QString& label, CircuitWidgetState state) -> QWidget* {
    const auto button = new ElementButton(label);
    button->setCheckable(true);
    button_map_[state] = button;

    connect(button, &QPushButton::clicked, this,
            [this, state]() { circuit_widget_->set_circuit_state(state); });

    return button;
}

auto line_separator() -> QWidget* {
    const auto line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

auto TopWidget::build_element_buttons() -> QWidget* {
    const auto layout = new QGridLayout();
    int row = -1;

    {
        const auto ES = [](DefaultMouseAction action) {
            return CircuitWidgetState {EditingState {.default_mouse_action = action}};
        };
        using enum DefaultMouseAction;

        layout->addWidget(new_button("BTN", ES(insert_button)), ++row, 0);
        layout->addWidget(new_button("Wire", ES(insert_wire)), row, 1);
        layout->addWidget(new_button("LED", ES(insert_led)), ++row, 0);
        layout->addWidget(new_button("TXT", ES(insert_decoration_text_element)), row, 1);
        layout->addWidget(new_button("NUM", ES(insert_display_number)), ++row, 0);
        layout->addWidget(new_button("ASCII", ES(insert_display_ascii)), row, 1);
        layout->addWidget(line_separator(), ++row, 0, 1, 2);

        layout->addWidget(new_button("AND", ES(insert_and_element)), ++row, 0);
        layout->addWidget(new_button("NAND", ES(insert_nand_element)), row, 1);
        layout->addWidget(new_button("OR", ES(insert_or_element)), ++row, 0);
        layout->addWidget(new_button("NOR", ES(insert_nor_element)), row, 1);
        layout->addWidget(new_button("BUF", ES(insert_buffer_element)), ++row, 0);
        layout->addWidget(new_button("INV", ES(insert_inverter_element)), row, 1);
        layout->addWidget(new_button("XOR", ES(insert_xor_element)), ++row, 0);
        layout->addWidget(line_separator(), ++row, 0, 1, 2);

        layout->addWidget(new_button("Latch", ES(insert_latch_d)), ++row, 0);
        layout->addWidget(new_button("FF", ES(insert_flipflop_d)), row, 1);
        layout->addWidget(new_button("MS-FF", ES(insert_flipflop_ms_d)), ++row, 0);
        layout->addWidget(new_button("JK-FF", ES(insert_flipflop_jk)), row, 1);
        layout->addWidget(line_separator(), ++row, 0, 1, 2);

        layout->addWidget(new_button("CLK", ES(insert_clock_generator)), ++row, 0);
        layout->addWidget(new_button("REG", ES(insert_shift_register)), row, 1);
    }

    layout->setRowStretch(++row, 1);

    const auto panel = new QWidget();
    panel->setLayout(layout);
    return panel;
}

void TopWidget::on_timer_update_title() {
    const auto statistics = circuit_widget_->statistics();

    auto text =
        fmt::format("[{}x{}] {:.1f} FPS {:.1f} pixel scale ({})", statistics.image_size.w,
                    statistics.image_size.h, statistics.frames_per_second,
                    statistics.pixel_scale, statistics.render_mode);

    if (statistics.simulation_events_per_second.has_value()) {
        const auto eps = statistics.simulation_events_per_second.value();
        text = fmt::format("{} {:.3g} EPS", text, round_fast(eps));
    }

    if (!last_saved_filename_.isEmpty()) {
        text = fmt::format("{} - {}", text, last_saved_filename_);
    }

    QString title = QString::fromStdString(text);
    if (title != windowTitle()) {
        setWindowTitle(title);
    }
}

void TopWidget::on_circuit_state_changed(CircuitWidgetState new_state) {
    bool simulation_active = is_simulation(new_state);

    // buttons
    for (auto&& [state, button] : button_map_) {
        if (button != nullptr) {
            button->setChecked(new_state == state);
        }
    }

    // delay slider
    if (delay_panel_ != nullptr) {
        delay_panel_->setEnabled(!simulation_active);
    }

    // simulation panel
    if (actions_.simulation_start != nullptr) {
        actions_.simulation_start->setEnabled(!simulation_active);
    }
    if (actions_.simulation_stop != nullptr) {
        actions_.simulation_stop->setEnabled(simulation_active);
    }
    if (actions_.wire_delay != nullptr) {
        actions_.wire_delay->setEnabled(!simulation_active);
    }
    if (actions_.wire_delay_checkbox != nullptr) {
        actions_.wire_delay_checkbox->setEnabled(!simulation_active);
    }

    // non-interactive
    actions_.non_interactive_mode->setChecked(is_non_interactive(new_state));
}

void TopWidget::on_timer_process_app_arguments_once() {
    for (const auto& argument : QCoreApplication::arguments() | std::views::drop(1)) {
        if (QFileInfo(argument).isFile()) {
            open_circuit(argument);
            break;
        }
    }
}

auto TopWidget::filename_filter() -> QString {
    return tr("Circuit Files (*.ls2);;All Files (*)");
}

auto TopWidget::new_circuit() -> void {
    if (ensure_circuit_saved() == save_result_t::success) {
        circuit_widget_->do_action(circuit_widget::UserAction::clear_circuit);
        circuit_widget_->do_action(circuit_widget::UserAction::reset_view);

        circuit_widget_->set_circuit_state(defaults::selection_state);
        circuit_widget_->set_simulation_config({});

        last_saved_filename_.clear();
        last_saved_data_ = circuit_widget_->serialized_circuit();
    }
}

auto TopWidget::save_circuit(filename_choice_t filename_choice) -> save_result_t {
    const auto filename = [&] {
        if (!last_saved_filename_.isEmpty() &&
            filename_choice == filename_choice_t::same_as_last) {
            return last_saved_filename_;
        }
        auto result = QFileDialog::getSaveFileName(this,              //
                                                   tr("Save As"),     //
                                                   "",                //
                                                   filename_filter()  //
        );
        if (result.endsWith(".ls2")) {
            return result;
        }
        if (result.endsWith(".")) {
            return result + "ls2";
        }
        return result + ".ls2";
    }();
    if (filename.isEmpty()) {
        return save_result_t::canceled;
    }

    const auto _ [[maybe_unused]] = Timer("Save");

    if (!circuit_widget_->save_circuit(filename)) {
        const auto message = fmt::format("Failed to save \"{}\".", filename);
        QMessageBox::warning(this,                                 //
                             QString::fromStdString(LS_APP_NAME),  //
                             QString::fromStdString(message)       //
        );
        return save_circuit(filename_choice_t::ask_new);
    }

    last_saved_filename_ = filename;
    last_saved_data_ = circuit_widget_->serialized_circuit();

    return save_result_t::success;
}

auto TopWidget::open_circuit(std::optional<QString> filename) -> void {
    if (ensure_circuit_saved() != save_result_t::success) {
        return;
    }

    if (!filename) {
        filename = QFileDialog::getOpenFileName(this,              //
                                                tr("Open"),        //
                                                "",                //
                                                filename_filter()  //
        );
    }

    if (!filename || filename->isEmpty()) {
        return;
    }

    const auto _ [[maybe_unused]] = Timer("Open");

    if (const auto error = circuit_widget_->load_circuit(*filename); error) {
        // Version Errors ask the users to update LogikSim to a specific version.
        // Those are the only ones a user can act upon. Log the rest.
        const auto suffix = error.value().type() == LoadErrorType::json_version_error  //
                                ? fmt::format("\n\n{}", error.value())
                                : "";
        const auto message = fmt::format("Failed to load \"{}\".{}", filename, suffix);

        print("WARNING: Failed to open:", filename);
        print(error.value().type());
        print(error.value().format());
        print();

        QMessageBox::warning(this,                                 //
                             QString::fromStdString(LS_APP_NAME),  //
                             QString::fromStdString(message)       //
        );
    } else {
        last_saved_filename_ = *filename;
        last_saved_data_ = circuit_widget_->serialized_circuit();
    }
}

auto TopWidget::load_circuit_example(int number) -> void {
    if (ensure_circuit_saved() == save_result_t::success) {
        circuit_widget_->load_circuit_example(number);

        last_saved_filename_.clear();
        last_saved_data_ = circuit_widget_->serialized_circuit();
    }
}

auto TopWidget::ensure_circuit_saved() -> save_result_t {
    if (last_saved_data_ == circuit_widget_->serialized_circuit()) {
        return save_result_t::success;
    }

    const auto name =
        last_saved_filename_.isEmpty() ? tr("New Circuit") : last_saved_filename_;
    const auto message = fmt::format("Save file \"{}\"?", name);
    const auto result = QMessageBox::question(
        this,                                                      //
        QString::fromStdString(LS_APP_NAME),                       //
        QString::fromStdString(message),                           //
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,  //
        QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        return save_circuit(filename_choice_t::same_as_last);
    }

    if (result == QMessageBox::No) {
        return save_result_t::success;
    }

    return save_result_t::canceled;
}

void TopWidget::on_simulation_config_changed(SimulationConfig new_config) {
    // simulation_time_rate
    set_time_rate_slider(new_config.simulation_time_rate);

    // use_wire_delay
    if (actions_.wire_delay != nullptr) {
        actions_.wire_delay->setChecked(new_config.use_wire_delay);
    }
}

Q_SLOT void TopWidget::on_render_config_changed(WidgetRenderConfig new_config) {
    if (actions_.do_benchmark != nullptr) {
        actions_.do_benchmark->setChecked(new_config.do_benchmark);
    }
    if (actions_.show_circuit != nullptr) {
        actions_.show_circuit->setChecked(new_config.show_circuit);
    }
    if (actions_.show_collision_cache != nullptr) {
        actions_.show_collision_cache->setChecked(new_config.show_collision_cache);
    }
    if (actions_.show_connection_cache != nullptr) {
        actions_.show_connection_cache->setChecked(new_config.show_connection_cache);
    }
    if (actions_.show_selection_cache != nullptr) {
        actions_.show_selection_cache->setChecked(new_config.show_selection_cache);
    }

    // wire render style
    {
        if (actions_.wire_render_style_red != nullptr) {
            actions_.wire_render_style_red->setChecked(new_config.wire_render_style ==
                                                       WireRenderStyle::red);
        }
        if (actions_.wire_render_style_bold != nullptr) {
            actions_.wire_render_style_bold->setChecked(new_config.wire_render_style ==
                                                        WireRenderStyle::bold);
        }
        if (actions_.wire_render_style_bold_red != nullptr) {
            actions_.wire_render_style_bold_red->setChecked(
                new_config.wire_render_style == WireRenderStyle::bold_red);
        }
    }

    // thread count
    {
        if (actions_.thread_count_synchronous != nullptr) {
            actions_.thread_count_synchronous->setChecked(new_config.thread_count ==
                                                          ThreadCount::synchronous);
        }
        if (actions_.thread_count_two != nullptr) {
            actions_.thread_count_two->setChecked(new_config.thread_count ==
                                                  ThreadCount::two);
        }
        if (actions_.thread_count_four != nullptr) {
            actions_.thread_count_four->setChecked(new_config.thread_count ==
                                                   ThreadCount::four);
        }
        if (actions_.thread_count_eight != nullptr) {
            actions_.thread_count_eight->setChecked(new_config.thread_count ==
                                                    ThreadCount::eight);
        }
    }

    if (actions_.show_render_borders != nullptr) {
        actions_.show_render_borders->setChecked(new_config.show_render_borders);
    }
    if (actions_.show_mouse_position != nullptr) {
        actions_.show_mouse_position->setChecked(new_config.show_mouse_position);
    }
    if (actions_.direct_rendering != nullptr) {
        actions_.direct_rendering->setChecked(new_config.direct_rendering);
    }
    if (actions_.jit_rendering != nullptr) {
        actions_.jit_rendering->setChecked(new_config.jit_rendering);
    }
}

auto TopWidget::set_time_rate_slider(time_rate_t time_rate) -> void {
    using namespace detail::time_slider;

    if (time_rate_slider_ != nullptr) {
        time_rate_slider_->setValue(to_slider_scale(time_rate));
    }
}

auto TopWidget::show_about_dialog() -> void {
    const auto text_fmt = tr("<h1>{}</h1>\n"
                             "<p>Version {}</p>"
                             "<p>Author: {}<br>"
                             "Contact: <a href=\"mailto:{}\">{}</a></p>"
                             "<p>© {}</p>")
                              .toStdString();
    const auto text =
        fmt::format(fmt::runtime(text_fmt), LS_APP_NAME, LS_APP_VERSION_STR,
                    LS_APP_AUTHOR, LS_APP_EMAIL, LS_APP_EMAIL, LS_APP_YEAR_STR);

    QMessageBox::about(this, tr("About"), QString::fromStdString(text));
}

auto TopWidget::save_gui_state() -> void {
    // geometry
    {
        const auto bytes = saveGeometry();
        const auto string =
            std::string {bytes.data(), gsl::narrow<std::size_t>(bytes.size())};
        save_file(get_writable_setting_path(setting_t::gui_geometry), string);
    }

    // state
    {
        const auto bytes = saveState();
        const auto string =
            std::string {bytes.data(), gsl::narrow<std::size_t>(bytes.size())};
        save_file(get_writable_setting_path(setting_t::gui_state), string);
    }

    // settings
    {
        const auto render_config = circuit_widget_->render_config();
        const auto settings = GuiSettings {
            .thread_count = render_config.thread_count,
            .wire_render_style = render_config.wire_render_style,
            .direct_rendering = render_config.direct_rendering,
            .jit_rendering = render_config.jit_rendering,
        };
        const auto string = serialize_gui_settings(settings);
        save_file(get_writable_setting_path(setting_t::gui_settings), string);
    }
}

namespace {

[[nodiscard]] auto load_gui_settings_from_file() -> tl::expected<GuiSettings, LoadError> {
    return load_file(get_writable_setting_path(setting_t::gui_settings))
        .and_then(load_gui_settings);
}

}  // namespace

auto TopWidget::restore_gui_state() -> void {
    // geometry
    if (const auto str = load_file(get_writable_setting_path(setting_t::gui_geometry));
        str) {
        const auto bytes = QByteArray {str->data(), gsl::narrow<qsizetype>(str->size())};
        restoreGeometry(bytes);
    }

    // state
    if (const auto str = load_file(get_writable_setting_path(setting_t::gui_state));
        str) {
        const auto bytes = QByteArray {str->data(), gsl::narrow<qsizetype>(str->size())};
        restoreState(bytes);
    }

    // settings
    if (const auto settings = load_gui_settings_from_file(); settings) {
        auto render_config = circuit_widget_->render_config();

        render_config.thread_count = settings->thread_count;
        render_config.wire_render_style = settings->wire_render_style;
        render_config.direct_rendering = settings->direct_rendering;
        render_config.jit_rendering = settings->jit_rendering;

        circuit_widget_->set_render_config(render_config);
    } else {
        print("WARNING: Unable to read GUI settings:", settings.error());
    }
}

auto TopWidget::closeEvent(QCloseEvent* event) -> void {
    event->ignore();

    if (ensure_circuit_saved() == save_result_t::success) {
        event->accept();
        save_gui_state();
    }
}

auto TopWidget::dragEnterEvent(QDragEnterEvent* event) -> void {
    const auto& mimeData = *event->mimeData();

    if (mimeData.hasUrls() && mimeData.urls().size() == 1 &&
        mimeData.urls().front().isLocalFile()) {
        event->acceptProposedAction();
    }
}

auto TopWidget::dropEvent(QDropEvent* event) -> void {
    const auto& mimeData = *event->mimeData();

    if (mimeData.hasUrls() && mimeData.urls().size() == 1 &&
        mimeData.urls().front().isLocalFile()) {
        const auto filename = mimeData.urls().front().toLocalFile();
        open_circuit(filename);
    }
}

}  // namespace logicsim
