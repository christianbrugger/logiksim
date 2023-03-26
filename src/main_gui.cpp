

#include "main_widget.h"

#include <QApplication>

auto main(int argc, char* argv[]) -> int {
    auto app = QApplication {argc, argv};

    auto frame = logicsim::MainWidget {};
    frame.show();

    return app.exec();
}
