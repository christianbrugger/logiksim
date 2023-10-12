#include "vocabulary/connector_info.h"

namespace logicsim {

auto connector_info_t::format() -> std::string {
    return fmt::format("<{}, {}>", position, orientation);
}

}  // namespace logicsim
