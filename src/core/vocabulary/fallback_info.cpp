#include "core/vocabulary/fallback_info.h"

#include "core/logging.h"

#include <fmt/core.h>

namespace logicsim {

auto fallback_info_t::format() const -> std::string {
    return fmt::format("{}", message);
}

fallback_info_t::operator bool() const {
    return !message.empty();
}

auto FallbackPrinter::print_if_set(std::string_view message,
                                   const fallback_info_t& info) -> void {
    if (bool {info} && info != last_info_) {
        print(message, info);
    }

    last_info_ = info;
}

}  // namespace logicsim
