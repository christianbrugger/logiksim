#include "core/vocabulary/decoration_key.h"

namespace logicsim {

auto decoration_key_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
