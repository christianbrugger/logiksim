#ifdef LS_LOG_TO_FILE
#include "gui/qt/setting_location.h"
#endif
#include "gui/widget/top_widget.h"

#ifdef LS_LOG_TO_FILE
#include "core/algorithm/u8_conversion.h"
#include "core/logging.h"
#endif
#include "core/resource.h"

#include <fmt/os.h>

#include <QApplication>
#include <QMessageBox>

auto main(int argc, char* argv[]) -> int {
    using namespace logicsim;

    QApplication::setApplicationName(LS_APP_NAME);
    QApplication::setApplicationVersion(LS_APP_VERSION_STR);
    QApplication::setOrganizationName(LS_APP_NAME);

    // Note, QApplication needs to be created before any QMessageBox is shown.
    auto app [[maybe_unused]] = QApplication {argc, argv};

#ifdef LS_EXCEPTION_MESSAGE_BOX
    try {
#endif
#ifdef LS_LOG_TO_FILE
        const auto log_file_path = get_writable_setting_path(setting_t::logfile);
        // TODO what happens if folder is not writable?
        // Note, this requires a valid utf-8 path for this to work on Windows.
        // TODO: check how this error is handled
        const auto log_file_str = to_string(log_file_path.u8string());
        const auto log_file [[maybe_unused]] = try_create_logfile(log_file_str);
#endif

        auto frame = TopWidget {};
        frame.show();
        return QApplication::exec();

#ifdef LS_EXCEPTION_MESSAGE_BOX
    }

    catch (const std::exception& exc) {
        QMessageBox::critical(nullptr, "Critical Error",
                              QString::fromStdString(exc.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Critical Error", QString("Unknown exception"));
        return -1;
    }
#endif
}
