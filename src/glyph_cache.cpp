#include "glyph_cache.h"

#include "exception.h"
#include "format.h"
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

auto glyph_key_t::format() const -> std::string {
    return fmt::format("({}, \"{}\")", font_size, text);
}

}  // namespace glyph_cache

GlyphCache::GlyphCache() {
    // TODO store text file in different location & make it configurable
    // const auto error = font_face_.createFromFile("Roboto-Regular.ttf");
    const auto error = font_face_.createFromFile("NotoSans-Regular.ttf");

    if (error) {
        // TODO create custom exception that can be handeled
        throw_exception("Could not find font face file");
    }
}

auto GlyphCache::get_font(float font_size) const -> const BLFont& {
    const auto [it, inserted] = font_map_.try_emplace(font_size);
    auto& font = it->second;

    if (inserted) {
        font.createFromFace(font_face_, font_size);
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

auto GlyphCache::get_glyph_entry(float font_size, const std::string& text,
                                 HorizontalAlignment horizontal_alignment,
                                 VerticalAlignment vertical_alignment) const
    -> const glyph_entry_t& {
    const auto [it, inserted] = glyph_map_.try_emplace(glyph_key_t {
        .font_size = font_size,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
        .text = text,
    });
    auto& glyph_entry = it->second;

    if (inserted) {
        const auto& font = get_font(font_size);

        glyph_entry.glyph_buffer.setUtf8Text(text.c_str(), text.size());
        font.shape(glyph_entry.glyph_buffer);
        // font.applyKerning(glyph_buffer);
        glyph_entry.offset = calculate_offset(font, glyph_entry.glyph_buffer,
                                              horizontal_alignment, vertical_alignment);
    }

    return glyph_entry;
}

auto GlyphCache::draw_text(BLContext& ctx, const BLPoint& position, float font_size,
                           const std::string& text, color_t color,
                           HorizontalAlignment horizontal_alignment,
                           VerticalAlignment vertical_alignment) const -> void {
    const auto& font = get_font(font_size);
    const auto& glyph_entry =
        get_glyph_entry(font_size, text, horizontal_alignment, vertical_alignment);

    ctx.fillGlyphRun(position - glyph_entry.offset, font,
                     glyph_entry.glyph_buffer.glyphRun(), color);
}

}  // namespace logicsim

/*
template <>
auto ankerl::unordered_dense::hash<logicsim::glyph_cache::glyph_key_t>::operator()(
    const logicsim::glyph_cache::glyph_key_t& obj) const noexcept -> uint64_t
*/