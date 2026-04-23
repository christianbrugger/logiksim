#include "core/vocabulary/length.h"

#include <fmt/format.h>

namespace logicsim {

auto length_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
