
#include "render_widget.h"

#include <gsl/gsl>

#include <QString>
#include <QWidget>

namespace logicsim {

class MainWidget : public QWidget {
    // TODO use Q_OBJECT because of Q_SLOT
    // Q_OBJECT
   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    [[nodiscard]] auto build_render_buttons() -> QWidget*;
    [[nodiscard]] auto build_mode_buttons() -> QWidget*;
    [[nodiscard]] auto build_time_rate_slider() -> QWidget*;
    [[nodiscard]] auto build_element_buttons() -> QWidget*;

    [[nodiscard]] auto element_button(QString label, LogicItemDefinition definition)
        -> QWidget*;

    auto update_selected_logic_item() -> void;

    void update_title();

   private:
    QTimer timer_;
    gsl::not_null<RendererWidget*> render_widget_;

    std::size_t input_count_ {3};
    LogicItemDefinition element_definition_ {};
};

}  // namespace logicsim
