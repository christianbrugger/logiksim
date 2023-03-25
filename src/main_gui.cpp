

#include "main_widget.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    auto frame = logicsim::MainWidget {};
    frame.show();

    return QApplication::exec();
}
