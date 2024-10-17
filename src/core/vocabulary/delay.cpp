#include "core/vocabulary/delay.h"

#include "core/format/time.h"

namespace logicsim {

auto delay_t::format() const -> std::string {
    const auto value_unsafe = std::chrono::duration<rep, period> {value.count()};
    return format_time(value_unsafe);
}

}  // namespace logicsim
