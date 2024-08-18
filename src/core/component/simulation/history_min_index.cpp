#include "component/simulation/history_min_index.h"

namespace logicsim {

namespace simulation {

auto history_min_index_t::format() const -> std::string {
    return fmt::format("{}", index);
}

}  // namespace simulation

}  // namespace logicsim
