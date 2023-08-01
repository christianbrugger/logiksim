
#include "render_widget.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <QString>
#include <QWidget>

class QSlider;
class QAbstractButton;

namespace logicsim {

class MainWidget : public QWidget {
    // TODO use Q_OBJECT because of Q_SLOT
    // Q_OBJECT
   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    [[nodiscard]] auto build_render_buttons() -> QWidget*;
    [[nodiscard]] auto build_mode_buttons() -> QWidget*;
    [[nodiscard]] auto build_delay_slider() -> QWidget*;
    [[nodiscard]] auto build_time_rate_slider() -> QWidget*;
    [[nodiscard]] auto build_element_buttons() -> QWidget*;

    [[nodiscard]] auto element_button(QString label, InteractionState state) -> QWidget*;

    Q_SLOT auto update_title() -> void;
    Q_SLOT auto on_interaction_state_changed(InteractionState new_state) -> void;

   private:
    gsl::not_null<RendererWidget*> render_widget_;

    QTimer timer_ {};

    using button_map_type
        = ankerl::unordered_dense::map<InteractionState, QAbstractButton*>;
    button_map_type button_map_ {};
    QSlider* delay_slider_ {};
};

}  // namespace logicsim
