#ifndef LOGIKSIM_CIRCUIT_DESCRIPTION_H
#define LOGIKSIM_CIRCUIT_DESCRIPTION_H

#include "vocabulary.h"

#include <string>
#include <vector>

namespace logicsim {

class CircuitDescription {
   public:
   private:
    circuit_id_t circuit_id_ {};
    std::string name_ {};
    std::size_t input_count {};
    std::size_t output_count {};
    color_t color_ {defaults::color_black};
    std::vector<std::string> input_names_ {};
    std::vector<std::string> output_names_ {};
};

}  // namespace logicsim

#endif
