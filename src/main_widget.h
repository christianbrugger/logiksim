
#include "render_widget.h"

#include <gsl/gsl>

#include <QWidget>

namespace logicsim {

class MainWidget : public QWidget {
    // TODO use Q_OBJECT because of Q_SLOT
    // Q_OBJECT
   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    auto build_render_buttons() -> QWidget*;
    auto build_mode_buttons() -> QWidget*;
    auto build_time_rate_slider() -> QWidget*;

    void update_title();

   private:
    QTimer timer_;
    gsl::not_null<RendererWidget*> render_widget_;
};

}  // namespace logicsim
