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
        case center:
            return "center";
        case top:
            return "top";
        case bottom:
            return "bottom";
    }
    throw_exception("Don't know how to convert VerticalAlignment to string.");
}

auto EmptyGlyphCache::draw_text(BLContext &ctx [[maybe_unused]],
                                const BLPoint &position [[maybe_unused]],
                                std::string_view text [[maybe_unused]],
                                float font_size [[maybe_unused]],
                                color_t color [[maybe_unused]],
                                HorizontalAlignment horizontal_alignment [[maybe_unused]],
                                VerticalAlignment vertical_alignment [[maybe_unused]],
                                FontStyle style [[maybe_unused]]) const -> void {};

}  // namespace logicsim