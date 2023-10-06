#include "delay.h"

#include "format/time.h"
#include "vocabulary/delay.h"

namespace logicsim {

auto delay_t::format() const -> std::string {
    // return format_microsecond_time(value);
    return format_time(value);
}

}  // namespace logicsim
