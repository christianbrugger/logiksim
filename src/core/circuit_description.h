#ifndef LOGIKSIM_CIRCUIT_DESCRIPTION_H
#define LOGIKSIM_CIRCUIT_DESCRIPTION_H

#include "core/vocabulary/circuit_id.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/connection_count.h"

#include <string>
#include <vector>

namespace logicsim {

struct CircuitDescription {
    circuit_id_t circuit_id {};
    std::string name {};
    connection_count_t input_count {};
    connection_count_t output_count {};
    color_t color {defaults::color_black};
    std::vector<std::string> input_names {};
    std::vector<std::string> output_names {};
};

}  // namespace logicsim

#endif
