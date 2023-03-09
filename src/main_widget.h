
#include "render_widget.h"

#include <gsl/gsl>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

namespace logicsim {

class MainWidget : public QWidget {
    // Q_OBJECT
   public:
    MainWidget(QWidget* parent = nullptr);

   private:
    auto build_buttons() -> QWidget*;
    void update_title();

   private:
    QTimer timer_;
    gsl::not_null<RendererWidget*> render_widget_;
};

}  // namespace logicsim
