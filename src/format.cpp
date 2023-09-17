#include "format.h"

//
// Logging
//

namespace logicsim {

LogFile::LogFile(fmt::cstring_view filename) : file_ {fmt::output_file(filename)} {
    if (logicsim::detail::format::file_stream != nullptr) {
        throw_exception("already using a different log file");
    }
    logicsim::detail::format::file_stream = &file_;
}

LogFile::~LogFile() {
    logicsim::detail::format::file_stream = nullptr;
}

auto try_create_logfile(fmt::cstring_view filename) -> std::optional<LogFile> {
    try {
        return std::optional<LogFile>(filename);
    } catch (const std::system_error &exc) {
        print("Could not setup file logging:", exc.what());
    }
    return std::nullopt;
}

}  // namespace logicsim
