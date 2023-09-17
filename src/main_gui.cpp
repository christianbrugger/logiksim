

#include "format.h"
#include "main_widget.h"
#include "resource.h"

#include <fmt/os.h>

#include <QApplication>
#include <QMessageBox>

auto main(int argc, char* argv[]) -> int {
    using namespace logicsim;
    auto app = QApplication {argc, argv};

#ifdef LS_LOG_TO_FILE
    const auto log_filename = to_absolute_path("logging.txt").toStdString();
    const auto log_file [[maybe_unused]] = try_create_logfile(log_filename);
#endif

#ifdef LS_EXCEPTION_MESSAGE_BOX
    try {
#endif

        auto frame = MainWidget {};
        frame.show();
        return app.exec();

#ifdef LS_EXCEPTION_MESSAGE_BOX
    }

    catch (std::runtime_error& exc) {
        print(exc.what());

        QMessageBox::critical(nullptr, "Critical Error", QString(exc.what()));

        return -1;
    }
#endif
}
