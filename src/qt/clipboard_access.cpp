#include "qt/clipboard_access.h"

#include <QApplication>
#include <QClipboard>
#include <QString>

namespace logicsim {

auto get_clipboard_text() -> std::string {
    return QApplication::clipboard()->text().toStdString();
}

auto set_clipboard_text(const std::string &text) -> void {
    QApplication::clipboard()->setText(QString::fromStdString(text));
}

}  // namespace logicsim
