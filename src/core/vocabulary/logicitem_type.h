#ifndef LOGICSIM_VOCABULARY_LOGICITEM_TYPE_H
#define LOGICSIM_VOCABULARY_LOGICITEM_TYPE_H

#include "format/enum.h"

#include <array>
#include <cstdint>

namespace logicsim {

/**
 * @brief: The type an logic-item of the circuit & layout is.
 */
enum class LogicItemType : uint8_t {
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

    //
    // Types that are not representable in schematic & simulation last
    //

    text_element,
};

template <>
[[nodiscard]] auto format(LogicItemType type) -> std::string;

constexpr inline auto all_logicitem_types = std::array {
    LogicItemType::buffer_element,  //
    LogicItemType::and_element,     //
    LogicItemType::or_element,      //
    LogicItemType::xor_element,     //

    LogicItemType::button,          //
    LogicItemType::led,             //
    LogicItemType::display_number,  //
    LogicItemType::display_ascii,   //

    LogicItemType::clock_generator,  //
    LogicItemType::flipflop_jk,      //
    LogicItemType::shift_register,   //
    LogicItemType::latch_d,          //
    LogicItemType::flipflop_d,       //
    LogicItemType::flipflop_ms_d,    //

    LogicItemType::sub_circuit,  //

    LogicItemType::text_element,  //
};

}  // namespace logicsim

#endif
