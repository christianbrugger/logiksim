#include "glyph_cache_type.h"

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
    throw_exception("Don't know how to convert FontStyle to string.");
}

template <>
auto format(HorizontalAlignment alignment) -> std::string {
    switch (alignment) {
        using enum HorizontalAlignment;

        case left:
            return "left";
        case right:
            return "right";
        case center:
            return "center";
    }
    throw_exception("Don't know how to convert HorizontalAlignment to string.");
}

template <>
auto format(VerticalAlignment alignment) -> std::string {
    switch (alignment) {
        using enum VerticalAlignment;

        case baseline:
            return "baseline";

        case center_baseline:
            return "center_baseline";
        case top_baseline:
            return "top_baseline";
        case bottom_baseline:
            return "bottom_baseline";

        case center:
            return "center";
        case top:
            return "top";
        case bottom:
            return "bottom";
    }
    throw_exception("Don't know how to convert VerticalAlignment to string.");
}

}  // namespace logicsim