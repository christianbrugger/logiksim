#ifndef LOGIKSIM_CIRCUIT_DESCRIPTION_H
#define LOGIKSIM_CIRCUIT_DESCRIPTION_H

#include "vocabulary.h"

#include <string>
#include <vector>

namespace logicsim {

struct CircuitDescription {
    circuit_id_t circuit_id {};
    std::string name {};
    std::size_t input_count {};
    std::size_t output_count {};
    color_t color {defaults::color_black};
    std::vector<std::string> input_names {};
    std::vector<std::string> output_names {};
};

}  // namespace logicsim

#endif
