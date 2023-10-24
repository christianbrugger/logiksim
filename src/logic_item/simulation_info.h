#ifndef LOGICSIM_LOGIC_ITEM_SIMULATION_INFO_H
#define LOGICSIM_LOGIC_ITEM_SIMULATION_INFO_H

#include "vocabulary/element_type.h"
#include "vocabulary/logic_small_vector.h"

#include <vector>

namespace logicsim {

struct connection_count_t;
class Schematic;

[[nodiscard]] auto has_no_logic(const ElementType type) noexcept -> bool;

[[nodiscard]] auto internal_state_size(const ElementType type) -> std::size_t;

[[nodiscard]] auto has_internal_state(const ElementType type) -> bool;

//
// Initialization
//

/**
 * brief: Initializes the simulation input values for the given schematic.
 *
 * Method will not change the sizes of input_values vector.
 */
auto initialize_input_values(const Schematic &schematic,
                             std::vector<logic_small_vector_t> &input_values) -> void;

//
// Simulation Behavior
//

auto update_internal_state(const logic_small_vector_t &old_input,
                           const logic_small_vector_t &new_input, const ElementType type,
                           logic_small_vector_t &state) -> void;

[[nodiscard]] auto calculate_outputs_from_state(const logic_small_vector_t &state,
                                                connection_count_t output_count,
                                                const ElementType type)
    -> logic_small_vector_t;

[[nodiscard]] auto calculate_outputs_from_inputs(const logic_small_vector_t &input,
                                                 connection_count_t output_count,
                                                 const ElementType type)
    -> logic_small_vector_t;

}  // namespace logicsim

#endif
