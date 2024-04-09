#include "vocabulary/selection_id.h"

#include <fmt/core.h>

namespace logicsim {

auto selection_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
