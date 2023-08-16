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

auto calculate_offset(const BLFont& font, BLGlyphBuffer& glyph_buffer,
                      HorizontalAlignment horizontal_alignment,
                      VerticalAlignment vertical_alignment) -> BLPoint {
    BLPoint result {};

    BLFontMetrics font_metrics = font.metrics();
    BLTextMetrics text_metrics;
    font.getTextMetrics(glyph_buffer, text_metrics);

    switch (horizontal_alignment) {
        using enum HorizontalAlignment;

        case left:
            result.x = text_metrics.boundingBox.x0;
            break;
        case right:
            result.x = text_metrics.boundingBox.x1;
            break;
        case center:
            result.x = (text_metrics.boundingBox.x0 + text_metrics.boundingBox.x1) / 2.0;
            break;
    }

    switch (vertical_alignment) {
        using enum VerticalAlignment;

        case baseline:
            break;
        case center:
            result.y = (font_metrics.descent - font_metrics.ascent) / 2.0;
            break;
        case top:
            result.y = -font_metrics.ascent;
            break;
        case bottom:
            result.y = font_metrics.descent;
            break;
    }

    return result;
}

auto GlyphCache::get_glyph_entry(std::string_view text, float font_size,
                                 HorizontalAlignment horizontal_alignment,
                                 VerticalAlignment vertical_alignment) const
    -> const glyph_entry_t& {
    const auto [it, inserted] = glyph_map_.try_emplace(glyph_key_t {
        .text_hash = glyph_cache::hash(text),
        .font_size = font_size,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
    });
    auto& glyph_entry = it->second;

    if (inserted) {
        const auto& font = get_font(font_size);

        glyph_entry.shaped_text = HarfbuzzShapedText {text, hb_font_face_, font_size};
        glyph_entry.glyph_buffer.setUtf8Text(text.data(), text.size());
        font.shape(glyph_entry.glyph_buffer);
        glyph_entry.offset = calculate_offset(font, glyph_entry.glyph_buffer,
                                              horizontal_alignment, vertical_alignment);
    }

    return glyph_entry;
}

auto GlyphCache::draw_text(BLContext& ctx, const BLPoint& position, std::string_view text,
                           float font_size, color_t color,
                           HorizontalAlignment horizontal_alignment,
                           VerticalAlignment vertical_alignment) const -> void {
    if (text.empty()) {
        return;
    }

    const auto& font = get_font(font_size);
    print("unitsPerEm", font.unitsPerEm());
    const auto& glyph_entry =
        get_glyph_entry(text, font_size, horizontal_alignment, vertical_alignment);

    const auto glyph_run = glyph_entry.shaped_text.glyph_run();
    // const auto& glyph_run = glyph_entry.glyph_buffer.glyphRun();

    ctx.fillGlyphRun(position - glyph_entry.offset, font, glyph_run, color);
}

}  // namespace logicsim
