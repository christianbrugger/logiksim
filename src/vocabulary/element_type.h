#ifndef LOGICSIM_VOCABULARY_ELEMENT_TYPE_H
#define LOGICSIM_VOCABULARY_ELEMENT_TYPE_H

#include "format/enum.h"

#include <cstdint>

namespace logicsim {

/**
 * @brief: The type an element of the circuit is.
 */
enum class ElementType : uint8_t {
    unused,       // has no inputs and no logic
    placeholder,  // output-placeholder - has no logic - only stores output values
    wire,

    buffer_element,
    and_element,
    or_element,
    xor_element,

    button,
    led,
    display_number,
    display_ascii,

    clock_generator,
    flipflop_jk,
    shift_register,
    latch_d,
    flipflop_d,
    flipflop_ms_d,  // ms = master slave

    sub_circuit,
};

template <>
auto format(ElementType type) -> std::string;

[[nodiscard]] auto is_logic_item(ElementType element_type) -> bool;

}  // namespace logicsim

#endif
