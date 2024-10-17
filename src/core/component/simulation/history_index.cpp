#include "core/component/simulation/history_index.h"

#include <fmt/core.h>

namespace logicsim {

namespace simulation {

auto history_index_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace simulation

}  // namespace logicsim
