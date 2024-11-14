#include "core/vocabulary/segment_key.h"

namespace logicsim {

auto segment_key_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
