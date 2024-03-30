#include "logging.h"

#include <cstdio>
#include <stdexcept>

namespace logicsim {

namespace {
thread_local inline fmt::ostream *logfile_stream = nullptr;
}

LogFile::LogFile(fmt::cstring_view filename) : file_ {fmt::output_file(filename)} {
    if (logfile_stream != nullptr) {
        throw std::runtime_error("already using a different log file");
    }
    logfile_stream = &file_;
}

LogFile::~LogFile() {
    logfile_stream = nullptr;
}

[[nodiscard]] auto try_create_logfile(fmt::cstring_view filename)
    -> std::optional<LogFile> {
    try {
        return std::optional<LogFile>(filename);
    } catch (const std::system_error &exc) {
        print("Could not setup file logging:", exc.what());
    }
    return std::nullopt;
}

namespace detail {

auto get_logfile_stream() noexcept -> fmt::ostream * {
    return logfile_stream;
}

}  // namespace detail

auto print_flush() -> void {
    std::fflush(stdout);
}

}  // namespace logicsim
