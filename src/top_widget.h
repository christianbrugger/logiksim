#ifndef LOGIKSIM_TOP_WIDGET_H
#define LOGIKSIM_TOP_WIDGET_H

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
constexpr static auto inline LS_APP_YEAR_STR = " 2023 ";

class CircuitWidget;
struct time_rate_t;

class ElementButton : public QPushButton {
    Q_OBJECT
   public:
    explicit ElementButton(const QString& text, QWidget* parent = nullptr);
    auto sizeHint() const -> QSize override;
    auto minimumSizeHint() const -> QSize override;
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
};

class MainWidget : public QMainWindow {
    Q_OBJECT

   public:
    using CircuitState = circuit_widget::CircuitState;
    using RenderConfig = circuit_widget::RenderConfig;
    using SimulationConfig = circuit_widget::SimulationConfig;

   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    auto create_menu() -> void;
    auto create_toolbar() -> void;
    auto create_statusbar() -> void;

    [[nodiscard]] auto build_element_buttons() -> QWidget*;

    [[nodiscard]] auto element_button(QString label, CircuitState state) -> QWidget*;

    Q_SLOT void update_title();
    Q_SLOT void on_circuit_state_changed(CircuitState new_state);
    Q_SLOT void on_simulation_config_changed(SimulationConfig new_config);
    Q_SLOT void on_render_config_changed(RenderConfig new_config);

    auto process_arguments() -> void;

    auto filename_filter() const -> QString;
    auto new_circuit() -> void;
    enum class filename_choice_t { ask_new, same_as_last };
    enum class save_result_t { success, canceled };
    auto save_circuit(filename_choice_t filename_choice) -> save_result_t;
    auto open_circuit(std::optional<std::string> filename = {}) -> void;
    auto ensure_circuit_saved() -> save_result_t;
    auto set_time_rate_slider(time_rate_t time_rate) -> void;
    auto show_about_dialog() -> void;

    auto save_gui_state() -> void;
    auto restore_gui_state() -> void;

   protected:
    auto closeEvent(QCloseEvent* event) -> void;
    auto dragEnterEvent(QDragEnterEvent* event) -> void;
    auto dropEvent(QDropEvent* event) -> void;

   private:
    gsl::not_null<CircuitWidget*> circuit_widget_;

    QTimer timer_ {};
    QTimer timer_process_arguments_ {};

    using button_map_type = ankerl::unordered_dense::map<CircuitState, QAbstractButton*>;
    button_map_type button_map_ {};

    QWidget* delay_panel_ {};
    QSlider* delay_slider_ {};

    std::string last_saved_filename_ {};
    std::string last_saved_data_ {};

    MainActions actions_ {};
    QMenu* menu_toolbars_ {};
    QSlider* time_rate_slider_ {};
};

}  // namespace logicsim

#endif
