#include "vocabulary/time_rate.h"

#include "format/time.h"

namespace logicsim {

auto time_rate_t::format() const -> std::string {
    return fmt::format("{}/s", rate_per_second);
}

}  // namespace logicsim
