#ifndef LOGICSIM_CORE_VOCABULARY_THEME_STYLE_H
#define LOGICSIM_CORE_VOCABULARY_THEME_STYLE_H

#include "core/format/enum.h"

#include <string>

namespace logicsim {

/**
 * @brief: Theme style in wich the circuit is drawn.
 */
enum class ThemeStyle : uint8_t {
    light,
    dark,
};

template <>
[[nodiscard]] auto format(ThemeStyle style) -> std::string;

/**
 * @brief: User request for the selected theme style.
 */
enum class ThemeStyleRequest : uint8_t {
    system_default,
    light,
    dark,
};

template <>
[[nodiscard]] auto format(ThemeStyleRequest style) -> std::string;

}  // namespace logicsim

#endif
