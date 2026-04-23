#include "core/vocabulary/element_id.h"

#include <fmt/format.h>

namespace logicsim {

auto element_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
