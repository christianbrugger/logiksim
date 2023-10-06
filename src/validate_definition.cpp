#include "validate_definition.h"

#include "layout_calculation.h"
#include "vocabulary/element_definition.h"
#include "vocabulary/element_type.h"

namespace logicsim {

auto is_valid(const attributes_clock_generator_t& a) -> bool {
    return a.time_symmetric > delay_t {0ns} &&  //
           a.time_on > delay_t {0ns} &&         //
           a.time_off > delay_t {0ns};
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
        connection_count_t {d.input_inverters.size()} != d.input_count) {
        return false;
    }
    if (!d.output_inverters.empty() &&
        connection_count_t {d.output_inverters.size()} != d.output_count) {
        return false;
    }

    // clock generator
    if (d.element_type == clock_generator) {
        if (!d.attrs_clock_generator || !is_valid(*d.attrs_clock_generator)) {
            return false;
        }
    }

    return true;
}

}  // namespace logicsim
