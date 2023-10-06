#include "time.h"

#include "format/time.h"
#include "vocabulary/time.h"

namespace logicsim {

auto time_t::format() const -> std::string {
    const auto value_unsafe = std::chrono::duration<rep, period> {value.count()};
    return format_microsecond_time(value_unsafe);
}

}  // namespace logicsim
