#include "vocabulary/element_definition.h"

#include <fmt/core.h>

namespace logicsim {

auto attributes_clock_generator_t::format() const -> std::string {
    const auto time_str = is_symmetric
                              ? fmt::format("time={}", 2 * time_symmetric)
                              : fmt::format("time_on={}, time_off={}", time_on, time_off);
    return fmt::format(
        "attributes_clock_generator("
        "name={}, {}, show_controls={}"
        ")",
        name, time_str, show_simulation_controls);
}

auto ElementDefinition::format() const -> std::string {
    const auto attr_str =
        attrs_clock_generator
            ? fmt::format(", attrs_clock_generator={}", *attrs_clock_generator)
            : std::string {};

    return fmt::format(
        "ElementDefinition("
        "element_type={}, input_count={}, output_count={}, "
        "orientation={}, circuit_id={}, input_inverters={}, "
        "output_inverters={}{}"
        ")",
        element_type, input_count, output_count, orientation, circuit_id, input_inverters,
        output_inverters, attr_str);
}

}  // namespace logicsim
