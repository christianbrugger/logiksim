#include "glyph_cache.h"

#include "exception.h"
#include "format.h"
#include "text_shaping.h"
#include "vocabulary.h"

namespace logicsim {

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

namespace glyph_cache {

auto hash(std::string_view text) noexcept -> uint64_t {
    return ankerl::unordered_dense::hash<std::string_view> {}(text);
}

auto glyph_key_t::format() const -> std::string {
    return fmt::format("({}, {}, {}, {})", text_hash, font_size, horizontal_alignment,
                       vertical_alignment);
}

auto glyph_entry_t::format() const -> std::string {
    return fmt::format("({}, {})", offset.x, offset.y);
}

}  // namespace glyph_cache

GlyphCache::GlyphCache() : GlyphCache(std::string {defaults::font_file}) {}

GlyphCache::GlyphCache(const std::string& font_file) : hb_font_face_ {font_file} {
    const auto status = bl_font_face_.createFromFile(font_file.c_str());
    if (status != BL_SUCCESS) {
        // TODO create custom exception that can be handeled
        throw_exception(fmt::format("Font not found {}", font_file).c_str());
    }
}

auto GlyphCache::format() const -> std::string {
    return fmt::format("GlyphCache(glyphs = {})", glyph_map_);
}

auto GlyphCache::get_font(float font_size) const -> const BLFont& {
    const auto [it, inserted] = font_map_.try_emplace(font_size);
    auto& font = it->second;

    if (inserted) {
        font.createFromFace(bl_font_face_, font_size);
    }

    return font;
}

auto calculate_offset(const HarfbuzzShapedText& text,
                      HorizontalAlignment horizontal_alignment,
                      VerticalAlignment vertical_alignment) -> BLPoint {
    BLPoint result {};

    const auto box = text.bounding_box();

    switch (horizontal_alignment) {
        using enum HorizontalAlignment;

        case left:
            result.x = box.x0;
            break;
        case right:
            result.x = box.x1;
            break;
        case center:
            result.x = (box.x0 + box.x1) / 2.0;
            break;
    }

    switch (vertical_alignment) {
        using enum VerticalAlignment;

        case baseline:
            break;
        case center:
            result.y = (box.y0 + box.y1) / 2.0;
            break;
        case top:
            result.y = box.y0;
            break;
        case bottom:
            result.y = box.y1;
            break;
    }

    return result;
}

auto GlyphCache::get_entry(std::string_view text, float font_size,
                           HorizontalAlignment horizontal_alignment,
                           VerticalAlignment vertical_alignment) const
    -> const glyph_entry_t& {
    const auto [it, inserted] = glyph_map_.try_emplace(glyph_key_t {
        .text_hash = glyph_cache::hash(text),
        .font_size = font_size,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
    });
    auto& entry = it->second;

    if (inserted) {
        entry.shaped_text = HarfbuzzShapedText {text, hb_font_face_, font_size};
        entry.offset =
            calculate_offset(entry.shaped_text, horizontal_alignment, vertical_alignment);
    }

    return entry;
}

auto GlyphCache::draw_text(BLContext& ctx, const BLPoint& position, std::string_view text,
                           float font_size, color_t color,
                           HorizontalAlignment horizontal_alignment,
                           VerticalAlignment vertical_alignment) const -> void {
    if (text.empty()) {
        return;
    }

    const auto& font = get_font(font_size);
    const auto& entry =
        get_entry(text, font_size, horizontal_alignment, vertical_alignment);
    const auto origin = position - entry.offset;

    ctx.fillGlyphRun(origin, font, entry.shaped_text.glyph_run(), color);

    // ctx.setStrokeWidth(1);
    // ctx.translate(origin);
    // ctx.strokeRect(entry.shaped_text.bounding_rect(), defaults::color_lime);
    // ctx.translate(-origin);
}

}  // namespace logicsim
