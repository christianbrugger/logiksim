#include "core/vocabulary/move_delta.h"

namespace logicsim {

auto move_delta_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

}  // namespace logicsim
