#include "vocabulary/logicitem_definition.h"

#include "allocated_size/std_string.h"

#include <fmt/core.h>

namespace logicsim {

auto attributes_clock_generator_t::format_period() const -> std::string {
    return is_symmetric ? fmt::format("{}", 2 * time_symmetric)
                        : fmt::format("{}/{}", time_on, time_off);
}

auto attributes_clock_generator_t::format() const -> std::string {
    return fmt::format("<clock: {}, {}, show_controls={}>", name, format_period(),
                       show_simulation_controls);
}

auto attributes_clock_generator_t::allocated_size() const -> std::size_t {
    return get_allocated_size(name);
}

auto LogicItemDefinition::format() const -> std::string {
    const auto attr_str =
        attrs_clock_generator
            ? fmt::format(", attrs_clock_generator={}", *attrs_clock_generator)
            : std::string {};

    return fmt::format(
        "LogicItemDefinition("
        "{}x{} {}, {}, "
        "sub_circuit_id={}, input_inverters={}, output_inverters={}{}"
        ")",
        input_count, output_count, logicitem_type, orientation,  //
        sub_circuit_id, input_inverters, output_inverters, attr_str);
}

}  // namespace logicsim
