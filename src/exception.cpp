#include "exception.h"

#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

namespace detail {

constexpr static inline auto enable_exception_traceback = false;

[[noreturn]] auto throw_exception_impl(const char *msg) -> void {
    const auto full_msg = [&] {
        return fmt::format("\nException: {}\n", msg);
    }();

    throw std::runtime_error(full_msg.c_str());
}

}  // namespace detail

}  // namespace logicsim
