

#include "format.h"
#include "main_widget.h"

#include <QApplication>
#include <QMessageBox>

#include <fstream>
#include <iostream>

auto main_impl(int argc, char* argv[]) -> int {
    auto app = QApplication {argc, argv};

    auto frame = logicsim::MainWidget {};
    frame.show();

    return app.exec();
}

auto main(int argc, char* argv[]) -> int {
#ifdef LS_EXCEPTION_MESSAGE_BOX
    try {
#endif

        return main_impl(argc, argv);

#ifdef LS_EXCEPTION_MESSAGE_BOX
    }

    catch (std::runtime_error& exc) {
        fmt::print("{}\n", exc.what());

        auto _ [[maybe_unused]] = QApplication {argc, argv};
        QMessageBox::critical(nullptr, "Critical Error", QString(exc.what()));

        return -1;
    }
#endif
}
