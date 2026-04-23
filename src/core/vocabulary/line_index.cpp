#include "core/vocabulary/line_index.h"

#include <fmt/format.h>

namespace logicsim {

auto line_index_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
