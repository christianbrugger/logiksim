#ifndef LOGIKSIM_GLYPH_CACHE_TYPE_H
#define LOGIKSIM_GLYPH_CACHE_TYPE_H

#include "format.h"
#include "vocabulary.h"

#include <string_view>

class BLContext;
struct BLPoint;

namespace logicsim {

enum class FontStyle : uint8_t {
    regular,
    italic,
    bold,
};

template <>
auto format(FontStyle style) -> std::string;

enum class HorizontalAlignment : uint8_t {
    left,
    right,
    center,
};

template <>
auto format(HorizontalAlignment alignment) -> std::string;

enum class VerticalAlignment : uint8_t {
    baseline,
    center,
    top,
    bottom,
};

template <>
auto format(VerticalAlignment alignment) -> std::string;

struct EmptyGlyphCache {
    auto draw_text(BLContext &ctx, const BLPoint &position, std::string_view text,
                   float font_size, color_t color = defaults::color_black,
                   HorizontalAlignment horizontal_alignment = HorizontalAlignment::left,
                   VerticalAlignment vertical_alignment = VerticalAlignment::baseline,
                   FontStyle style = FontStyle::regular) const -> void;
};

}  // namespace logicsim

#endif