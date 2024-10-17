#include "core/vocabulary/font_style.h"

#include <exception>

namespace logicsim {

template <>
auto format(FontStyle style) -> std::string {
    switch (style) {
        using enum FontStyle;

        case regular:
            return "regular";
        case italic:
            return "italic";
        case bold:
            return "bold";
        case monospace:
            return "monospace";
    }
    std::terminate();
}

}  // namespace logicsim
