#ifndef LOGICSIM_GUI_WIDGET_DEBUG_INFO_DIALOG_H
#define LOGICSIM_GUI_WIDGET_DEBUG_INFO_DIALOG_H

#include <QWidget>

class QTextEdit;

namespace logicsim {

struct CircuitWidgetAllocInfo;

class DebugInfoDialog : public QWidget {
   public:
    explicit DebugInfoDialog(QWidget* parent);

    auto update_allocation_info(const CircuitWidgetAllocInfo& info) -> void;

   private:
    QTextEdit* text_edit_ {};
};

}  // namespace logicsim

#endif
