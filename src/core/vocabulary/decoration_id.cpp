#include "core/vocabulary/decoration_id.h"

namespace logicsim {

auto decoration_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
