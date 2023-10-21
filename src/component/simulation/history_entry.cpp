#include "component/simulation/history_entry.h"

#include <fmt/core.h>

namespace logicsim {

namespace simulation {

auto history_entry_t::format() const -> std::string {
    return fmt::format("HistoryEntry({}, {}, {})", first_time, last_time, value);
}

}  // namespace simulation

}  // namespace logicsim
