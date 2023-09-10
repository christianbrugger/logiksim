#ifndef LOGIKSIM_MAIN_WIDGET_H
#define LOGIKSIM_MAIN_WIDGET_H

#include "render_widget_type.h"

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

namespace logicsim {

class RendererWidget;

class ElementButton : public QPushButton {
    Q_OBJECT
   public:
    explicit ElementButton(const QString& text, QWidget* parent = nullptr);
    auto sizeHint() const -> QSize override;
    auto minimumSizeHint() const -> QSize override;
};

constexpr static inline auto app_name = std::string_view {"LogicSim 2"};

class MainWidget : public QMainWindow {
    Q_OBJECT

   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    auto create_menu() -> void;

    [[nodiscard]] auto build_delay_slider() -> QWidget*;
    [[nodiscard]] auto build_time_rate_slider() -> QWidget*;
    [[nodiscard]] auto build_element_buttons() -> QWidget*;

    [[nodiscard]] auto element_button(QString label, InteractionState state) -> QWidget*;

    Q_SLOT void update_title();
    Q_SLOT void on_interaction_state_changed(InteractionState new_state);

    auto process_arguments() -> void;

    auto filename_filter() const -> QString;
    auto new_circuit() -> void;
    enum class filename_choice_t { ask_new, same_as_last };
    enum class save_result_t { success, canceled };
    auto save_circuit(filename_choice_t filename_choice) -> save_result_t;
    auto open_circuit(std::optional<std::string> filename = {}) -> void;
    auto ensure_circuit_saved() -> save_result_t;

   protected:
    auto closeEvent(QCloseEvent* event) -> void;
    auto dragEnterEvent(QDragEnterEvent* event) -> void;
    auto dropEvent(QDropEvent* event) -> void;

   private:
    gsl::not_null<RendererWidget*> render_widget_;

    QTimer timer_ {};
    QTimer timer_process_arguments_ {};

    using button_map_type =
        ankerl::unordered_dense::map<InteractionState, QAbstractButton*>;
    button_map_type button_map_ {};

    QWidget* delay_panel_ {};
    QSlider* delay_slider_ {};

    std::string last_saved_filename_ {};
    std::string last_saved_data_ {};
};

}  // namespace logicsim

#endif
