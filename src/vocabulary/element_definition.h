#ifndef LOGICSIM_VOCABULARY_ELEMENT_DEFINITION_H
#define LOGICSIM_VOCABULARY_ELEMENT_DEFINITION_H

#include "vocabulary/circuit_id.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/delay.h"
#include "vocabulary/element_type.h"
#include "vocabulary/logic_small_vector.h"
#include "vocabulary/orientation.h"

#include <optional>
#include <string>

namespace logicsim {

struct attributes_clock_generator_t {
    std::string name {"clock"};

    delay_t time_symmetric {500us};
    delay_t time_on {500us};
    delay_t time_off {500us};

    bool is_symmetric {true};
    bool show_simulation_controls {true};
};

struct ElementDefinition {
    ElementType element_type {ElementType::or_element};
    connection_count_t input_count {3};
    connection_count_t output_count {1};
    orientation_t orientation {orientation_t::right};

    circuit_id_t circuit_id {null_circuit};
    logic_small_vector_t input_inverters {};
    logic_small_vector_t output_inverters {};

    std::optional<attributes_clock_generator_t> attrs_clock_generator {};
};

}  // namespace logicsim

#endif
