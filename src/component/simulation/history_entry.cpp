#include "history_entry.h"

#include "component/simulation/history_entry.h"

#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

namespace simulation {

history_entry_t::history_entry_t(time_t first_time_, time_t last_time_, bool value_)
    : first_time {first_time_}, last_time {last_time_}, value {value_} {
    if (!(first_time_ < last_time_)) [[unlikely]] {
        throw std::runtime_error("first time nees to be smaller then last time");
    }
}

auto history_entry_t::format() const -> std::string {
    return fmt::format("HistoryEntry({}, {}, {})", first_time, last_time, value);
}

}  // namespace simulation

}  // namespace logicsim
