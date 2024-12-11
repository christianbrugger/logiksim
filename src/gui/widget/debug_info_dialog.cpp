#include "gui/widget/debug_info_dialog.h"

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

    edit->setText("<b>Heading</b><br/>Content");

    resize(300, 400);
}

}  // namespace logicsim
