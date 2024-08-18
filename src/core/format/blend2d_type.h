#ifndef LOGICSIM_FORMAT_BLEND2D_TYPE_H
#define LOGICSIM_FORMAT_BLEND2D_TYPE_H

#include <blend2d.h>
#include <fmt/core.h>

//
// BLPoint
//

template <typename Char>
struct fmt::formatter<BLPoint, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const BLPoint &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "({}, {})", obj.x, obj.y);
    }
};

//
// BLPointI
//

template <typename Char>
struct fmt::formatter<BLPointI, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const BLPointI &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "({}, {})", obj.x, obj.y);
    }
};

//
// BLBox
//

template <typename Char>
struct fmt::formatter<BLBox, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const BLBox &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "BLBox(({}, {}), ({}, {}))", obj.x0, obj.y0,
                              obj.x1, obj.y1);
    }
};

//
// BLRectI
//

template <typename Char>
struct fmt::formatter<BLRectI, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const BLRectI &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "BLRectI(x={}, y={}, w={}, h={})", obj.x, obj.y,
                              obj.w, obj.h);
    }
};

//
// BLSizeI
//

template <typename Char>
struct fmt::formatter<BLSizeI, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const BLSizeI &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "BLSizeI(w={}, h={})", obj.w, obj.h);
    }
};

//
// BLGlyphPlacement
//

template <typename Char>
struct fmt::formatter<BLGlyphPlacement, Char> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const BLGlyphPlacement &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "BLGlyphPlacement(placement = {}, advance = {})",
                              obj.placement, obj.advance);
    }
};

#endif
