#include "vocabulary/connector_info.h"

namespace logicsim {

auto simple_connector_info_t::format() -> std::string {
    return fmt::format("<{}, {}>", position, orientation);
}

auto extended_connector_info_t::format() -> std::string {
    return fmt::format("<{}: {}, {}>", id, position, orientation);
}

}  // namespace logicsim
