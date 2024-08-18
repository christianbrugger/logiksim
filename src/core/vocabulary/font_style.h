#ifndef LOGICSIM_VOCABULARY_FONT_STYLE_H
#define LOGICSIM_VOCABULARY_FONT_STYLE_H

#include "format/enum.h"

#include <array>
#include <cstdint>
#include <string>

namespace logicsim {

enum class FontStyle : uint8_t {
    regular,
    italic,
    bold,
    monospace,
};

constexpr inline auto all_font_styles = std::array {
    FontStyle::regular,
    FontStyle::italic,
    FontStyle::bold,
    FontStyle::monospace,
};

template <>
[[nodiscard]] auto format(FontStyle style) -> std::string;

}  // namespace logicsim

#endif
