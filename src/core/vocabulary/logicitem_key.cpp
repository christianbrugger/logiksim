#include "core/vocabulary/logicitem_key.h"

namespace logicsim {

auto logicitem_key_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
