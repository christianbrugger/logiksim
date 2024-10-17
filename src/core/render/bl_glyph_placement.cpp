#include "core/render/bl_glyph_placement.h"

auto operator==(const BLGlyphPlacement& a, const BLGlyphPlacement& b) -> bool {
    static_assert(sizeof(BLGlyphPlacement) == sizeof(BLGlyphPlacement::placement) +
                                                  sizeof(BLGlyphPlacement::advance));

    return a.placement == b.placement && a.advance == b.advance;
}
