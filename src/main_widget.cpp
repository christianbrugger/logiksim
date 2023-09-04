#include "main_widget.h"

#include "render_widget.h"
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
#include <QString>
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

    const auto layout = new QVBoxLayout();
    // layout->addWidget(build_render_buttons());
    // layout->addWidget(build_mode_buttons());
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

auto add_action_impl(QMenu* menu, const QString& text, action_callable auto callable)
    -> QAction* {
    auto* action = menu->addAction(text);
    auto* widget = menu->parentWidget();
    widget->connect(action, &QAction::triggered, widget, callable);
    return action;
}

auto add_action(QMenu* menu, const QString& text, std::invocable<> auto callable)
    -> QAction* {
    return add_action_impl(menu, text, callable);
}

auto add_action(QMenu* menu, const QString& text, const QKeySequence& shortcut,
                std::invocable<> auto callable) -> QAction* {
    auto* action = add_action(menu, text, callable);
    action->setShortcut(shortcut);
    action->setAutoRepeat(false);
    return action;
}

auto set_icon(QAction* action, const QString& icon) -> void {
    action->setIcon(QIcon("public/icons/lucide/" + icon));
}

auto add_action(QMenu* menu, const QString& text, const QKeySequence& shortcut,
                const QString& icon, std::invocable<> auto callable) -> QAction* {
    auto* action = add_action(menu, text, shortcut, callable);
    set_icon(action, icon);
    return action;
}

auto add_action(QMenu* menu, const QString& text, const QString& icon,
                std::invocable<> auto callable) -> QAction* {
    auto* action = add_action(menu, text, callable);
    set_icon(action, icon);
    return action;
}

auto add_action_checkable(QMenu* menu, const QString& text, bool start_state,
                          std::invocable<bool> auto callable) -> QAction* {
    auto* action = add_action_impl(menu, text, callable);
    action->setCheckable(true);
    action->setChecked(start_state);
    std::invoke(callable, start_state);
    return action;
}

auto add_action_group(QMenu* menu, const QString& text, QActionGroup* group,
                      bool start_state, std::invocable<> auto callable) -> QAction* {
    auto* action = add_action_impl(menu, text, callable);
    action->setActionGroup(group);
    action->setCheckable(true);
    action->setChecked(start_state);
    if (start_state) {
        std::invoke(callable);
    }
    return action;
}

}  // namespace

auto MainWidget::create_menu() -> void {
    // Browse Icons: https://lucide.dev/icons/
    {
        // File
        auto* menu = menuBar()->addMenu(tr("&File"));

        add_action(menu, tr("&New"), QKeySequence::New, "file.svg",
                   [this] { new_circuit(); });
        add_action(menu, tr("&Open..."), QKeySequence::Open, "folder-open.svg",
                   [this] { open_circuit(); });
        add_action(menu, tr("&Save"), QKeySequence::Save, "save.svg",
                   [this] { save_circuit(filename_choice_t::same_as_last); });
        add_action(menu, tr("Save &As..."), QKeySequence::SaveAs,
                   [this] { save_circuit(filename_choice_t::ask_new); });

        menu->addSeparator();
        add_action(menu, tr("E&xit"), QKeySequence::Quit, "log-out.svg",
                   [this]() { close(); });
    }

    {
        // Edit
        auto* menu = menuBar()->addMenu(tr("&Edit"));

        add_action(menu, tr("Cu&t"), QKeySequence::Cut, "scissors.svg",
                   [this] { render_widget_->cut_selected_items(); });
        add_action(menu, tr("&Copy"), QKeySequence::Copy, "copy.svg",
                   [this] { render_widget_->copy_selected_items(); });
        add_action(menu, tr("&Paste"), QKeySequence::Paste, "clipboard.svg",
                   [this] { render_widget_->paste_clipboard_items(); });
        add_action(menu, tr("&Delete"), QKeySequence::Delete, "trash-2.svg",
                   [this] { render_widget_->delete_selected_items(); });
        add_action(menu, tr("Select &All"), QKeySequence::SelectAll, "box-select.svg",
                   [this] { render_widget_->select_all_items(); });
        // select all options:
        //      "maximize.svg"
        //      "grid.svg"
        //      "check-square.svg"
        //      "box-select.svg"
    }
    {
        // Debug
        auto* menu = menuBar()->addMenu(tr("&Debug"));

        // Benchmark
        auto* ba = add_action_checkable(
            menu, tr("&Benchmark"), false,
            [this](bool checked) { render_widget_->set_do_benchmark(checked); });
        set_icon(ba, "infinity.svg");

        menu->addSeparator();
        {
            auto* ia = add_action_checkable(
                menu, tr("Show C&ircuit"), true,
                [this](bool checked) { render_widget_->set_do_render_circuit(checked); });
            set_icon(ia, "cpu.svg");
            auto* oa = add_action_checkable(
                menu, tr("Show C&ollision Cache"), false, [this](bool checked) {
                    render_widget_->set_do_render_collision_cache(checked);
                });
            set_icon(oa, "shapes.svg");
            auto* na = add_action_checkable(
                menu, tr("Show Co&nnection Cache"), false, [this](bool checked) {
                    render_widget_->set_do_render_connection_cache(checked);
                });
            set_icon(na, "spline.svg");  // share-2
            auto* sa = add_action_checkable(
                menu, tr("Show &Selection Cache"), false, [this](bool checked) {
                    render_widget_->set_do_render_selection_cache(checked);
                });
            set_icon(sa, "ungroup.svg");  // ungroup, group, boxes, ratio
        }

        // Examples
        menu->addSeparator();
        add_action(menu, tr("&Reload"), "refresh-ccw.svg",
                   [this]() { render_widget_->reload_circuit(); });
        {
            auto* ma = add_action(menu, tr("Load \"Si&mple\" Example"),
                                  [this]() { render_widget_->load_circuit_example(1); });
            set_icon(ma, "cable.svg");
            auto* wa = add_action(menu, tr("Load \"&Wires\" Example"),
                                  [this]() { render_widget_->load_circuit_example(4); });
            set_icon(wa, "share-2.svg");
            auto* ea = add_action(menu, tr("Load \"&Elements\" Example"),
                                  [this]() { render_widget_->load_circuit_example(3); });
            set_icon(ea, "workflow.svg");
            auto* ra = add_action(menu, tr("Load \"Elements + Wi&res\" Example"),
                                  [this]() { render_widget_->load_circuit_example(2); });
            set_icon(ra, "network.svg");
        }

        // Thread Count
        menu->addSeparator();
        auto* da = add_action_checkable(
            menu, tr("&Direct Rendering"), true,
            [this](bool checked) { render_widget_->set_use_backing_store(checked); });
        set_icon(da, "grid-2x2.svg");

        menu->addSeparator();
        {
            auto* group = new QActionGroup(menu);
            add_action_group(menu, tr("S&ynchronous Rendering"), group, false,
                             [this]() { render_widget_->set_thread_count(0); });
            add_action_group(menu, tr("&2 Render Threads"), group, false,
                             [this]() { render_widget_->set_thread_count(2); });
            add_action_group(menu, tr("&4 Render Threads"), group, true,
                             [this]() { render_widget_->set_thread_count(4); });
            add_action_group(menu, tr("&8 Render Threads"), group, false,
                             [this]() { render_widget_->set_thread_count(8); });
        }
    }
    {
        // Tools
        auto* menu = menuBar()->addMenu(tr("&Tools"));

        add_action(menu, tr("&Options..."), QKeySequence::Preferences, "settings.svg",
                   [] { print("options"); });
    }
}

auto MainWidget::build_render_buttons() -> QWidget* {
    const auto check_box1 = new QCheckBox("Benchmark");
    const auto check_box2 = new QCheckBox("Render Circuit");
    const auto check_box3 = new QCheckBox("Render Collision Cache");
    const auto check_box4 = new QCheckBox("Render Connection Cache");
    const auto check_box5 = new QCheckBox("Render Selection Cache");

    connect(check_box1, &QCheckBox::stateChanged, this, [this](int value) {
        render_widget_->set_do_benchmark(value == Qt::Checked);
    });
    connect(check_box2, &QCheckBox::stateChanged, this, [this](int value) {
        render_widget_->set_do_render_circuit(value == Qt::Checked);
    });
    connect(check_box3, &QCheckBox::stateChanged, this, [this](int value) {
        render_widget_->set_do_render_collision_cache(value == Qt::Checked);
    });
    connect(check_box4, &QCheckBox::stateChanged, this, [this](int value) {
        render_widget_->set_do_render_connection_cache(value == Qt::Checked);
    });
    connect(check_box5, &QCheckBox::stateChanged, this, [this](int value) {
        render_widget_->set_do_render_selection_cache(value == Qt::Checked);
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
    connect(button1, &QPushButton::clicked, this, [this](bool checked [[maybe_unused]]) {
        render_widget_->load_circuit_example(1);
    });
    connect(button2, &QPushButton::clicked, this, [this](bool checked [[maybe_unused]]) {
        render_widget_->load_circuit_example(2);
    });
    connect(button3, &QPushButton::clicked, this, [this](bool checked [[maybe_unused]]) {
        render_widget_->load_circuit_example(3);
    });
    connect(button4, &QPushButton::clicked, this, [this](bool checked [[maybe_unused]]) {
        render_widget_->load_circuit_example(4);
    });

    const auto threads_select = new QComboBox();
    static const auto available_counts = std::vector {0, 2, 4, 8};
    for (const auto& count : available_counts) {
        threads_select->addItem(QString::fromStdString(fmt::format("{}", count)),
                                QVariant(count));
    }
    connect(threads_select, &QComboBox::activated, this,
            [this, combo = threads_select](int index) {
                render_widget_->set_thread_count(combo->itemData(index).toInt());
            });
    threads_select->setCurrentIndex(2);

    const auto direct_checkbox = new QCheckBox("Direct Rendering");
    connect(direct_checkbox, &QCheckBox::stateChanged, this, [this](int value) {
        render_widget_->set_use_backing_store(value == Qt::Checked);
    });
    direct_checkbox->setCheckState(Qt::Checked);

    const auto layout = new QHBoxLayout();
    layout->addWidget(button0);
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);
    layout->addStretch(1);
    layout->addWidget(direct_checkbox);
    layout->addWidget(threads_select);
    layout->addWidget(new QLabel("threads"));

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

    // input count
    const auto input_count = new QSpinBox();
    layout->addWidget(input_count, ++row, 0, 1, 2);
    layout->addWidget(line_separator(), ++row, 0, 1, 2);

    connect(input_count, &QSpinBox::valueChanged, this,
            [this](int value) { render_widget_->set_default_input_count(value); });

    input_count->setValue(gsl::narrow<int>(render_widget_->default_input_count()));
    input_count->setMinimum(1);
    input_count->setMaximum(connection_id_t::max());

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
