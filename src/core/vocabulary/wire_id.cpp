#include "core/vocabulary/wire_id.h"

namespace logicsim {

auto wire_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
