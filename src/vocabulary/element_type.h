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

constexpr inline auto all_element_types = {
    ElementType::unused,       //
    ElementType::placeholder,  //
    ElementType::wire,         //

    ElementType::buffer_element,  //
    ElementType::and_element,     //
    ElementType::or_element,      //
    ElementType::xor_element,     //

    ElementType::button,          //
    ElementType::led,             //
    ElementType::display_number,  //
    ElementType::display_ascii,   //

    ElementType::clock_generator,  //
    ElementType::flipflop_jk,      //
    ElementType::shift_register,   //
    ElementType::latch_d,          //
    ElementType::flipflop_d,       //
    ElementType::flipflop_ms_d,    //

    ElementType::sub_circuit,  //
};

}  // namespace logicsim

#endif
