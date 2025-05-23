#ifndef LOGIKSIM_WIDGET_TOP_WIDGET_H
#define LOGIKSIM_WIDGET_TOP_WIDGET_H

#include "gui/widget/circuit_widget_base.h"

#include "core/serialize_gui_setting.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

class QString;
class QSlider;
class QAbstractButton;
class QHBoxLayout;
class QCheckBox;
class QMenu;
class QSlider;

namespace logicsim {

constexpr static auto inline LS_APP_AUTHOR = "Christian Brugger";
constexpr static auto inline LS_APP_EMAIL = "christian@rangetable.com";
constexpr static auto inline LS_APP_YEAR_STR = " 2024";
constexpr static auto inline LS_APP_WEBSITE =
    "https://github.com/christianbrugger/logiksim";
constexpr static auto inline LS_APP_LICENSE = "Apache 2.0";

struct time_rate_t;
class CircuitWidget;
class DebugInfoDialog;

class ElementButton : public QPushButton {
    Q_OBJECT
   public:
    explicit ElementButton(const QString& text, QWidget* parent = nullptr);
    [[nodiscard]] auto sizeHint() const -> QSize override;
    [[nodiscard]] auto minimumSizeHint() const -> QSize override;
};

struct MainActions {
    // file
    QAction* new_file;
    QAction* open_file;
    QAction* save_file;

    // edit
    QAction* undo;
    QAction* redo;
    QAction* cut;
    QAction* copy;
    QAction* paste;

    // simulation
    QAction* simulation_start;
    QAction* simulation_stop;
    QAction* wire_delay;
    QCheckBox* wire_delay_checkbox;

    // debug
    QAction* do_benchmark;
    QAction* show_circuit;
    QAction* show_collision_index;
    QAction* show_connection_index;
    QAction* show_selection_index;

    QAction* show_render_borders;
    QAction* show_mouse_position;
    QAction* non_interactive_mode;
    QAction* direct_rendering;
    QAction* jit_rendering;

    QAction* thread_count_synchronous;
    QAction* thread_count_two;
    QAction* thread_count_four;
    QAction* thread_count_eight;

    QAction* wire_render_style_red;
    QAction* wire_render_style_bold;
    QAction* wire_render_style_bold_red;
};

class TopWidget : public QMainWindow {
    Q_OBJECT

   public:
    explicit TopWidget(QWidget* parent = nullptr);

   private:
    auto create_menu() -> void;
    auto create_toolbar() -> void;
    auto create_statusbar() -> void;

    [[nodiscard]] auto build_element_buttons() -> QWidget*;
    [[nodiscard]] auto new_button(const QString& label,
                                  CircuitWidgetState state) -> QWidget*;

    // circuit_widget slots
    Q_SLOT void on_circuit_state_changed(CircuitWidgetState new_state);
    Q_SLOT void on_simulation_config_changed(SimulationConfig new_config);
    Q_SLOT void on_render_config_changed(WidgetRenderConfig new_config);
    Q_SLOT void on_history_status_changed(HistoryStatus new_status);
    // timer slots
    Q_SLOT void on_timer_update_title();
    Q_SLOT void on_timer_update_debug_info();
    Q_SLOT void on_timer_process_app_arguments_once();

    // complex setters
    auto set_time_rate_slider(time_rate_t time_rate) -> void;

    // complex actions
    auto new_circuit() -> void;
    auto show_about_dialog() -> void;
    auto show_debug_info_dialog() -> void;

    // load & safe
    [[nodiscard]] static auto filename_filter() -> QString;
    [[nodiscard]] static auto default_save_filepath() -> QString;
    enum class filename_choice_t { ask_new, same_as_last };
    enum class save_result_t { success, canceled };
    auto save_circuit(filename_choice_t filename_choice) -> save_result_t;
    auto open_circuit(std::optional<QString> filename = {}) -> void;
    auto load_circuit_example(int number) -> void;
    auto ensure_circuit_saved() -> save_result_t;

    // gui state
    auto save_gui_state() -> void;
    auto restore_gui_state() -> void;
    auto set_gui_settings(const GuiDebugSettings& settings) -> void;

   protected:
    auto closeEvent(QCloseEvent* event) -> void override;
    auto dragEnterEvent(QDragEnterEvent* event) -> void override;
    auto dropEvent(QDropEvent* event) -> void override;

   private:
    gsl::not_null<CircuitWidget*> circuit_widget_;
    gsl::not_null<QHBoxLayout*> circuit_widget_layout_;
    GuiDebugSettings debug_settings_ {};

    QTimer timer_update_title_ {};
    QTimer timer_process_app_arguments_once_ {};

    using button_map_type =
        ankerl::unordered_dense::map<CircuitWidgetState, QAbstractButton*>;
    button_map_type button_map_ {};

    QWidget* delay_panel_ {};
    QSlider* delay_slider_ {};

    QString last_saved_filename_ {};
    std::string last_saved_data_ {};

    MainActions actions_ {};
    QMenu* menu_toolbars_ {};
    QSlider* time_rate_slider_ {};
    QMenu* menu_debug_ {};

    QPointer<DebugInfoDialog> debug_info_dialog_ {};
};

}  // namespace logicsim

#endif
