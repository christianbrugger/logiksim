#include "time.h"

#include "format/time.h"
#include "vocabulary/time.h"

namespace logicsim {

auto time_t::format() const -> std::string {
    return format_microsecond_time(value);
}

}  // namespace logicsim
