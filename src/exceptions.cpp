#include "exceptions.h"

#include <boost/stacktrace.hpp>
#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

[[noreturn]] auto throw_exception(const char *msg) -> void {
    const auto stacktrace = boost::stacktrace::to_string(boost::stacktrace::stacktrace());
    const auto full_msg = fmt::format("{}\nException: {}\n", stacktrace, msg);
    throw std::runtime_error(full_msg.c_str());
}

}  // namespace logicsim
