#include "glyph_cache.h"

#include "exception.h"
#include "format.h"
#include "text_shaping.h"
#include "vocabulary.h"

namespace logicsim {

auto font_definition_t::get(FontStyle style) const -> std::string_view {
    switch (style) {
        using enum FontStyle;

        case regular:
            return this->regular;
        case italic:
            return this->italic;
        case bold:
            return this->bold;
    }
    throw_exception("unknown FontStyle");
}

namespace glyph_cache {

auto hash(std::string_view text) noexcept -> uint64_t {
    return ankerl::unordered_dense::hash<std::string_view> {}(text);
}

auto glyph_key_t::format() const -> std::string {
    return fmt::format("({}, {}, {}, {}, {})", text_hash, font_size, style,
                       horizontal_alignment, vertical_alignment);
}

auto glyph_entry_t::format() const -> std::string {
    return fmt::format("({}, {})", offset.x, offset.y);
}

}  // namespace glyph_cache

//
// Font Face
//

FontFace::FontFace(std::string font_file) : hb_font_face {font_file}, bl_font_face {} {
    const auto status = bl_font_face.createFromFile(font_file.c_str());
    if (status != BL_SUCCESS && !font_file.empty()) [[unlikely]] {
        // TODO create custom exception that can be handeled
        throw_exception(fmt::format("Font not found {}", font_file).c_str());
    }
}

//
// Font Faces
//

FontFaces::FontFaces(font_definition_t font_files)
    : regular {std::string {font_files.regular}},
      italic {std::string {font_files.italic}},
      bold {std::string {font_files.bold}} {}

auto FontFaces::get(FontStyle style) const -> const FontFace& {
    switch (style) {
        using enum FontStyle;

        case regular:
            return this->regular;
        case italic:
            return this->italic;
        case bold:
            return this->bold;
    }
    throw_exception("unknown FontStyle");
}

//
// Fonts
//

Fonts::Fonts(const FontFaces& font_faces) {
    const auto font_size = float {1.0};
    regular.createFromFace(font_faces.get(FontStyle::regular).bl_font_face, font_size);
    italic.createFromFace(font_faces.get(FontStyle::italic).bl_font_face, font_size);
    bold.createFromFace(font_faces.get(FontStyle::bold).bl_font_face, font_size);
}

auto Fonts::get(FontStyle style) -> BLFont& {
    switch (style) {
        using enum FontStyle;

        case regular:
            return this->regular;
        case italic:
            return this->italic;
        case bold:
            return this->bold;
    }
    throw_exception("unknown FontStyle");
}

//
// GlyphCache
//

namespace {

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

}  // namespace

GlyphCache::GlyphCache() : GlyphCache(font_definition_t {defaults::font_files}) {}

GlyphCache::GlyphCache(font_definition_t font_files)
    : font_faces_ {font_files}, fonts_ {font_faces_} {}

auto GlyphCache::format() const -> std::string {
    return fmt::format("GlyphCache({} glyphs)", glyph_map_.size());
}

auto GlyphCache::get_font(float font_size, FontStyle style) const -> const BLFont& {
    // reuse font, to avoid allocation everytime we draw a text
    auto& font = fonts_.get(style);
    font.setSize(font_size);
    return font;
}

auto GlyphCache::get_entry(std::string_view text, float font_size, FontStyle style,
                           HorizontalAlignment horizontal_alignment,
                           VerticalAlignment vertical_alignment) const
    -> const glyph_entry_t& {
    const auto [it, inserted] = glyph_map_.try_emplace(glyph_key_t {
        .text_hash = glyph_cache::hash(text),
        .font_size = font_size,
        .style = style,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
    });
    auto& entry = it->second;

    if (inserted) {
        const auto& font_face = font_faces_.get(style);

        entry.shaped_text = HarfbuzzShapedText {text, font_face.hb_font_face, font_size};
        entry.offset =
            calculate_offset(entry.shaped_text, horizontal_alignment, vertical_alignment);
    }

    return entry;
}

auto GlyphCache::draw_text(BLContext& ctx, const BLPoint& position, std::string_view text,
                           float font_size, TextAttributes attributes) const -> void {
    if (text.empty()) {
        return;
    }

    const auto& font = get_font(font_size, attributes.style);
    const auto& entry =
        get_entry(text, font_size, attributes.style, attributes.horizontal_alignment,
                  attributes.vertical_alignment);
    const auto origin = position - entry.offset;

    ctx.fillGlyphRun(origin, font, entry.shaped_text.glyph_run(), attributes.color);

    if constexpr (const bool debug_rect [[maybe_unused]] = false) {
        ctx.setStrokeWidth(1);
        ctx.translate(origin);
        ctx.strokeRect(entry.shaped_text.bounding_rect(), defaults::color_lime);
        ctx.translate(-origin);
    }
}

auto GlyphCache::draw_text(BLContext& ctx, const BLPoint& position, std::string_view text,
                           float font_size, color_t color,
                           HorizontalAlignment horizontal_alignment,
                           VerticalAlignment vertical_alignment, FontStyle style) const
    -> void {
    draw_text(ctx, position, text, font_size,
              TextAttributes {color, horizontal_alignment, vertical_alignment, style});
}

}  // namespace logicsim
