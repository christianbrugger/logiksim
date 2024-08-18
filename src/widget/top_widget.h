#ifndef LOGIKSIM_WIDGET_TOP_WIDGET_H
#define LOGIKSIM_WIDGET_TOP_WIDGET_H

#include "circuit_widget_base.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <QMainWindow>
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
constexpr static auto inline LS_APP_EMAIL = "brugger.chr@gmail.com";
constexpr static auto inline LS_APP_YEAR_STR = " 2024 ";

class CircuitWidget;
struct time_rate_t;

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
    QAction* show_collision_cache;
    QAction* show_connection_cache;
    QAction* show_selection_cache;

    QAction* show_render_borders;
    QAction* show_mouse_position;
    QAction* non_interactive_mode;
    QAction* direct_rendering;

    QAction* thread_count_0;
    QAction* thread_count_2;
    QAction* thread_count_4;
    QAction* thread_count_8;
};

class MainWidget : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWidget(QWidget* parent = nullptr);

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
    // timer slots
    Q_SLOT void on_timer_update_title();
    Q_SLOT void on_timer_process_app_arguments_once();

    // complex setters
    auto set_time_rate_slider(time_rate_t time_rate) -> void;

    // complex actions
    auto new_circuit() -> void;
    auto show_about_dialog() -> void;

    // load & safe
    [[nodiscard]] static auto filename_filter() -> QString;
    enum class filename_choice_t { ask_new, same_as_last };
    enum class save_result_t { success, canceled };
    auto save_circuit(filename_choice_t filename_choice) -> save_result_t;
    auto open_circuit(std::optional<QString> filename = {}) -> void;
    auto load_circuit_example(int number) -> void;
    auto ensure_circuit_saved() -> save_result_t;

    // gui state
    auto save_gui_state() -> void;
    auto restore_gui_state() -> void;

   protected:
    auto closeEvent(QCloseEvent* event) -> void override;
    auto dragEnterEvent(QDragEnterEvent* event) -> void override;
    auto dropEvent(QDropEvent* event) -> void override;

   private:
    gsl::not_null<CircuitWidget*> circuit_widget_;
    gsl::not_null<QHBoxLayout*> circuit_widget_layout_;

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
};

}  // namespace logicsim

#endif
