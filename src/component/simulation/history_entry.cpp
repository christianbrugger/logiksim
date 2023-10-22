#include "component/simulation/history_entry.h"

#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

namespace simulation {

history_entry_t::history_entry_t(New data)
    : history_entry_t {data.first_time, data.last_time, data.value} {}

history_entry_t::history_entry_t(time_t first_time_, time_t last_time_, bool value_)
    : first_time {first_time_}, last_time {last_time_}, value {value_} {
    if (!(first_time < last_time)) [[unlikely]] {
        throw std::runtime_error("first time needs to be smaller then last time");
    }
}

auto history_entry_t::format() const -> std::string {
    return fmt::format("HistoryEntry({}, {}, {})", first_time, last_time, value);
}

}  // namespace simulation

}  // namespace logicsim
