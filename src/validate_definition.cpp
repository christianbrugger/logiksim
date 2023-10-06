#include "validate_definition.h"

#include "layout_calculation.h"
#include "validate_definition.h"
#include "vocabulary/delay.h"
#include "vocabulary/element_definition.h"
#include "vocabulary/element_type.h"

namespace logicsim {

auto clock_generator_min_time() -> delay_t {
    return delay_t::epsilon();
}

auto clock_generator_max_time() -> delay_t {
    return delay_t {std::chrono::seconds {500}};
}

namespace {
[[nodiscard]] auto valid_clock_generator_time(delay_t time) -> bool {
    assert(clock_generator_max_time() > clock_generator_min_time());
    return clock_generator_min_time() <= time && time <= clock_generator_max_time();
}
}  // namespace

auto is_valid(const attributes_clock_generator_t& a) -> bool {
    return valid_clock_generator_time(a.time_symmetric) &&
           valid_clock_generator_time(a.time_on) &&
           valid_clock_generator_time(a.time_off);
}

auto is_valid(const ElementDefinition& d) -> bool {
    using enum ElementType;

    if (!is_input_output_count_valid(d.element_type, d.input_count, d.output_count)) {
        return false;
    }
    if (!is_orientation_valid(d.element_type, d.orientation)) {
        return false;
    }
    if (!d.input_inverters.empty() &&
        d.input_inverters.size() != std::size_t {d.input_count}) {
        return false;
    }
    if (!d.output_inverters.empty() &&
        d.output_inverters.size() != std::size_t {d.output_count}) {
        return false;
    }

    // clock generator
    if ((d.element_type == clock_generator) != d.attrs_clock_generator.has_value()) {
        return false;
    }
    if (d.attrs_clock_generator.has_value() && !is_valid(*d.attrs_clock_generator)) {
        return false;
    }

    return true;
}

}  // namespace logicsim
