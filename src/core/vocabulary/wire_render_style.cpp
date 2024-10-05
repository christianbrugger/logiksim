#include "vocabulary/wire_render_style.h"

namespace logicsim {

template <>
auto format(WireRenderStyle style) -> std::string {
    switch (style) {
        using enum WireRenderStyle;

        case red:
            return "red";
        case bold:
            return "bold";
        case bold_red:
            return "bold_red";
    };
    std::terminate();
}

}  // namespace logicsim
