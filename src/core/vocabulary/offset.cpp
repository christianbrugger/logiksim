#include "core/vocabulary/offset.h"

#include <fmt/format.h>

namespace logicsim {

auto offset_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
