#ifndef LOGICSIM_CORE_VOCABULARY_DECORATION_TYPE_H
#define LOGICSIM_CORE_VOCABULARY_DECORATION_TYPE_H

#include "core/format/enum.h"

#include <array>
#include <cstdint>

namespace logicsim {

/**
 * @brief: The type a decoration in the layout and circuit.
 */
enum class DecorationType : uint8_t {
    text_element,
};

template <>
[[nodiscard]] auto format(DecorationType type) -> std::string;

constexpr inline auto all_decoration_types = std::array {
    DecorationType::text_element,  //
};

}  // namespace logicsim

#endif
