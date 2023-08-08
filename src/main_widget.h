

#include "render_widget_types.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <QPushButton>
#include <QTimer>
#include <QWidget>

class QString;
class QSlider;
class QAbstractButton;

namespace logicsim {

class RendererWidget;

class ElementButton : public QPushButton {
    Q_OBJECT
   public:
    explicit ElementButton(const QString& text, QWidget* parent = nullptr);
    auto sizeHint() const -> QSize override;
    auto minimumSizeHint() const -> QSize override;
};

class MainWidget : public QWidget {
    Q_OBJECT

   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    [[nodiscard]] auto build_render_buttons() -> QWidget*;
    [[nodiscard]] auto build_mode_buttons() -> QWidget*;
    [[nodiscard]] auto build_delay_slider() -> QWidget*;
    [[nodiscard]] auto build_time_rate_slider() -> QWidget*;
    [[nodiscard]] auto build_element_buttons() -> QWidget*;

    [[nodiscard]] auto element_button(QString label, InteractionState state) -> QWidget*;

    Q_SLOT void update_title();
    Q_SLOT void on_interaction_state_changed(InteractionState new_state);

   private:
    gsl::not_null<RendererWidget*> render_widget_;

    QTimer timer_ {};

    using button_map_type =
        ankerl::unordered_dense::map<InteractionState, QAbstractButton*>;
    button_map_type button_map_ {};

    QWidget* delay_panel_ {};
    QSlider* delay_slider_ {};
};

}  // namespace logicsim
