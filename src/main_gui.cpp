

#include "format.h"
#include "main_widget.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>

auto main(int argc, char* argv[]) -> int {
    auto app = QApplication {argc, argv};

    // TODO logging
    const auto directory = app.applicationDirPath();
    if (!QDir::setCurrent(directory)) {
    }

    auto frame = logicsim::MainWidget {};
    frame.show();

    return app.exec();
}
