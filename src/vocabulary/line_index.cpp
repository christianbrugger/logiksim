#include "vocabulary/line_index.h"

#include <fmt/core.h>

namespace logicsim {

auto line_index_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
