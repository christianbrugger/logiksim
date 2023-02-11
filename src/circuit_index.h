#ifndef LOGIKSIM_CIRCUIT_INDEX_H
#define LOGIKSIM_CIRCUIT_INDEX_H

#include "circuit_description.h"
#include "circuit_layout.h"
#include "schematic.h"

#include <vector>

namespace logicsim {

class CircuitIndex {
   public:
    // Tasks:
    // * borrow/return_schematic(&&)
    // * borrow/return_schematics(&&)
    // * borrow/return_layout(&&)
   private:
    std::vector<Schematic> schematics_;
    std::vector<CircuitLayout> layouts_;
    std::vector<CircuitDescription> descriptions_;
};
}  // namespace logicsim

#endif
