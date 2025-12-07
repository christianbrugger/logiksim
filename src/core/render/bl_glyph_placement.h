#ifndef LOGIKSIM_RENDER_GL_GLYPH_PLACEMENT_H
#define LOGIKSIM_RENDER_GL_GLYPH_PLACEMENT_H

#include <blend2d/blend2d.h>

#include <concepts>

// for now the library does not provide an equality operator,
// if this changes, remove this module
static_assert(!std::equality_comparable<BLGlyphPlacement>);

auto operator==(const BLGlyphPlacement& a, const BLGlyphPlacement& b) -> bool;

#endif
