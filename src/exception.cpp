#include "exception.h"

#include <boost/stacktrace.hpp>
#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

namespace detail {

constexpr static inline auto enable_exception_traceback = false;

[[noreturn]] auto throw_exception_impl(const char *msg) -> void {
    const auto full_msg = [&] {
        if constexpr (enable_exception_traceback) {
            const auto tr = boost::stacktrace::to_string(boost::stacktrace::stacktrace());
            return fmt::format("{}\nException: {}\n", tr, msg);
        } else {
            return fmt::format("\nException: {}\n", msg);
        }
    }();

    throw std::runtime_error(full_msg.c_str());
}

}  // namespace detail

}  // namespace logicsim
