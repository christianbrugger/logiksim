#include "core/vocabulary/selection_id.h"

#include <fmt/format.h>

namespace logicsim {

auto selection_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
