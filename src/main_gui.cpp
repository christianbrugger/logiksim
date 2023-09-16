

#include "format.h"
#include "main_widget.h"
#include "resource.h"

#include <fmt/os.h>

#include <QApplication>
#include <QMessageBox>

namespace logicsim {

class LogFile {
   public:
    [[nodiscard]] explicit LogFile(fmt::cstring_view filename)
        : file_ {fmt::output_file(filename)} {
        logicsim::detail::format::file_stream = &file_;
    }

    ~LogFile() {
        logicsim::detail::format::file_stream = nullptr;
    }

    LogFile(LogFile&&) = default;
    LogFile(const LogFile&) = delete;
    auto operator=(LogFile&&) -> LogFile& = delete;
    auto operator=(const LogFile&) -> LogFile& = delete;

   private:
    fmt::ostream file_;
};

[[nodiscard]] auto try_create_logfile(fmt::cstring_view filename)
    -> std::optional<LogFile> {
    try {
        return std::optional<LogFile>(filename);
    } catch (const std::system_error& exc) {
        logicsim::print("Could not setup file logging:", exc.what());
    }
    return std::nullopt;
}

}  // namespace logicsim

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
