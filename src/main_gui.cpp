

#include "format.h"
#include "main_widget.h"

#include <QApplication>
#include <QDir>

auto main(int argc, char* argv[]) -> int {
    {
        auto app = QApplication {argc, argv};

        auto frame = logicsim::MainWidget {};
        frame.show();

        return app.exec();
    }

    // catch (std::runtime_error& exc) {
    //     fmt::print("{}\n", exc.what());
    //     throw;
    // }
}
