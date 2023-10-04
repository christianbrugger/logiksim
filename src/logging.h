#ifndef LOGICSIM_LOGGING_H
#define LOGICSIM_LOGGING_H

#include "format/container.h"
#include "format/qt_type.h"
#include "format/std_type.h"

#include <fmt/core.h>
#include <fmt/os.h>

#include <array>
#include <optional>

namespace logicsim {

class LogFile {
   public:
    /**
     * @brief: Create logfile at given location that is used for all prints.
     *
     * Note log files are thread-local.
     *
     * Raises runtime_error, if another log file already exists.
     * Raises system_error, if log file cannot be created.
     */
    [[nodiscard]] explicit LogFile(fmt::cstring_view filename);
    ~LogFile();

    // move construction only, same as fmt::ostream
    [[nodiscard]] LogFile(LogFile &&) = default;
    LogFile(const LogFile &) = delete;
    auto operator=(LogFile &&) -> LogFile & = delete;
    auto operator=(const LogFile &) -> LogFile & = delete;

   private:
    fmt::ostream file_;
};

/**
 * @brief: Tries to create a logfile at given location that is used for all prints.
 *
 * Raises runtime_error, if another log file already exists.
 * Returns nullopt if it fails to open the file.
 */
[[nodiscard]] auto try_create_logfile(fmt::cstring_view filename)
    -> std::optional<LogFile>;

namespace detail {
[[nodiscard]] auto get_logfile_stream() noexcept -> fmt::ostream *;
}

//
// print_fmt
//

/**
 * @brief: Log fmt specifier and args to logfile or stdout.
 */
template <typename... T>
auto print_fmt(fmt::format_string<T...> fmt, T &&...args) -> void {
    auto *file_stream = detail::get_logfile_stream();

    if (file_stream == nullptr) {
        fmt::print(fmt, std::forward<T>(args)...);
    } else {
        file_stream->print(fmt, std::forward<T>(args)...);
    }
}

//
// print
//

namespace detail {
// repeats {} count times: 2 -> "{} {}\n"
template <std::size_t count>
constexpr auto repeat_format_string() {
    constexpr auto buffer_size = count == 0 ? 1 : count * 3;
    std::array<char, buffer_size> buffer;

    std::size_t pos = 0;
    for (std::size_t i = 0; i < count; ++i) {
        std::string_view format = "{} ";
        for (char c : format) {
            buffer[pos++] = c;
        }
    }
    if constexpr (count > 0) {
        --pos;
    }
    buffer[pos] = '\n';
    return buffer;
}

}  // namespace detail

/**
 * @brief: Print each args separated by a space.
 *
 * Note each arg is formatted with {}.
 */
template <typename... Args>
auto print(Args &&...args) -> void {
    constexpr static std::size_t count = sizeof...(Args);
    constexpr static auto buffer = detail::repeat_format_string<count>();

    constexpr static auto fmt_str = std::string_view {buffer.data(), buffer.size()};
    print_fmt(fmt_str, std::forward<Args>(args)...);
}

}  // namespace logicsim

#endif
