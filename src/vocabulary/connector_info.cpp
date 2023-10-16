#include "vocabulary/connector_info.h"

namespace logicsim {

auto simple_input_info_t::format() const -> std::string {
    return fmt::format("<Input {}, {}>", position, orientation);
}

auto simple_output_info_t::format() const -> std::string {
    return fmt::format("<Output {}, {}>", position, orientation);
}

auto extended_input_info_t::format() const -> std::string {
    return fmt::format("<Input-{}: {}, {}>", input_id, position, orientation);
}

auto extended_output_info_t::format() const -> std::string {
    return fmt::format("<Output-{}: {}, {}>", output_id, position, orientation);
}

}  // namespace logicsim
