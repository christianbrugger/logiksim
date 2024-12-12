#include "gui/widget/debug_info_dialog.h"

#include "core/vocabulary/allocation_info.h"

#include <QTextEdit>
#include <QVBoxLayout>

namespace logicsim {

DebugInfoDialog::DebugInfoDialog(QWidget* parent) : QWidget {parent} {
    setWindowFlags(Qt::Dialog);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Debug Info Dialog"));

    const auto layout = new QVBoxLayout {this};
    const auto edit = new QTextEdit {this};
    layout->addWidget(edit);
    edit->setReadOnly(true);
    edit->setFontFamily("monospace");
    text_edit_ = edit;

    resize(400, 650);
}

auto DebugInfoDialog::update_allocation_info(const CircuitWidgetAllocInfo& info) -> void {
    if (text_edit_ == nullptr) {
        return;
    }

    text_edit_->setText(QString::fromStdString(info.format()));
}

}  // namespace logicsim
