#include "gui/widget/top_widget.h"

#include "core/logging.h"
#include "core/resource.h"

#include <fmt/os.h>

#include <QApplication>
#include <QMessageBox>

auto main(int argc, char* argv[]) -> int {
    using namespace logicsim;

    QApplication::setApplicationName(LS_APP_NAME);
    QApplication::setApplicationVersion(LS_APP_VERSION_STR);
    QApplication::setOrganizationName(LS_APP_NAME);

    auto app [[maybe_unused]] = QApplication {argc, argv};

#ifdef LS_LOG_TO_FILE
    const auto log_filename = get_writable_setting_path(setting_t::logfile).toStdString();
    const auto log_file [[maybe_unused]] = try_create_logfile(log_filename);
#endif

#ifdef LS_EXCEPTION_MESSAGE_BOX
    try {
#endif

        auto frame = MainWidget {};
        frame.show();
        return QApplication::exec();

#ifdef LS_EXCEPTION_MESSAGE_BOX
    }

    catch (std::runtime_error& exc) {
        print(exc.what());

        QMessageBox::critical(nullptr, "Critical Error", QString(exc.what()));

        return -1;
    }
#endif
}
