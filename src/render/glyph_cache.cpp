#include "render/glyph_cache.h"

#include "algorithm/fmt_join.h"
#include "file.h"
#include "font_style_property.h"
#include "logging.h"
#include "resource.h"

#include <fmt/core.h>

#include <exception>

namespace logicsim {

auto font_locations_t::get(FontStyle style) const -> const std::filesystem::path& {
    return ::logicsim::get<const std::filesystem::path&>(*this, style);
}

auto get_default_font_locations() -> font_locations_t {
    return font_locations_t {
        .regular = get_font_path(font_t::regular),
        .italic = get_font_path(font_t::italic),
        .bold = get_font_path(font_t::bold),
        .monospace = get_font_path(font_t::monospace),
    };
}

namespace glyph_cache {

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

FontFace::FontFace(std::filesystem::path font_file)
    : font_data_ {font_file.empty() ? "" : load_file(font_file)},
      hb_font_face_ {std::span<const char> {font_data_.data(), font_data_.size()}} {
    if (!font_file.empty() && font_data_.empty()) {
        print("WARNING: could not open font file", font_file);
    }

    {
        const auto status =
            bl_font_data_.createFromData(font_data_.data(), font_data_.size());
        if (!font_data_.empty() && status != BL_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not create BLFontData");
        }
    }
    {
        const auto status = bl_font_face_.createFromData(bl_font_data_, 0);
        if (!font_data_.empty() && status != BL_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not create BLFontFace");
        }
    }
}

auto FontFace::hb_font_face() const -> const HarfbuzzFontFace& {
    return hb_font_face_;
}

auto FontFace::bl_font_face() const -> const BLFontFace& {
    return bl_font_face_;
}

//
// Font Faces
//

FontFaces::FontFaces(font_locations_t font_files)
    : regular {FontFace {font_files.regular}},
      italic {FontFace {font_files.italic}},
      bold {FontFace {font_files.bold}},
      monospace {FontFace {font_files.monospace}} {}

auto FontFaces::get(FontStyle style) const -> const FontFace& {
    return ::logicsim::get<const FontFace&>(*this, style);
}

//
// Fonts
//

Fonts::Fonts(const FontFaces& font_faces) {
    const auto create_font_size = float {10};  // doesn't matter, as we rescale it later

    for (auto& style : all_font_styles) {
        get(style).createFromFace(font_faces.get(style).bl_font_face(), create_font_size);
    }
}

auto Fonts::get(FontStyle style) const -> const BLFont& {
    return ::logicsim::get<const BLFont&>(*this, style);
}

auto Fonts::get(FontStyle style) -> BLFont& {
    return ::logicsim::get<BLFont&>(*this, style);
}

//
// Stable Center Offsets
//

auto BaselineOffset::format() const -> std::string {
    return fmt::format(
        "BaselineOffset(baseline_center = {}, baseline_top = {}, "
        "baseline_bottom = {})",
        baseline_center, baseline_top, baseline_bottom);
}

[[nodiscard]] auto BaselineOffset::operator*(float font_size) const
    -> ScaledBaselineOffset {
    return ScaledBaselineOffset {
        .baseline_center = baseline_center * font_size,
        .baseline_top = baseline_top * font_size,
        .baseline_bottom = baseline_bottom * font_size,
    };
}

auto ScaledBaselineOffset::format() const -> std::string {
    return fmt::format(
        "ScaledBaselineOffset(baseline_center = {}, baseline_top = {}, "
        "baseline_bottom = {})",
        baseline_center, baseline_top, baseline_bottom);
}

namespace {

auto calculate_horizontal_offset(const BLBox& bounding_box,
                                 HTextAlignment horizontal_alignment) -> double {
    const auto& box = bounding_box;

    switch (horizontal_alignment) {
        using enum HTextAlignment;

        case left:
            return box.x0;
        case right:
            return box.x1;
        case center:
            return (box.x0 + box.x1) / 2.0;
    }
    std::terminate();
}

auto calculate_vertical_offset(const BLBox& bounding_box,
                               const ScaledBaselineOffset& baseline_offsets,
                               VTextAlignment vertical_alignment) -> double {
    const auto& box = bounding_box;

    switch (vertical_alignment) {
        using enum VTextAlignment;

        case baseline:
            return 0;

        case center_baseline:
            return baseline_offsets.baseline_center;
        case top_baseline:
            return baseline_offsets.baseline_top;
        case bottom_baseline:
            return baseline_offsets.baseline_bottom;

        case center:
            return (box.y0 + box.y1) / 2.0;
        case top:
            return box.y0;
        case bottom:
            return box.y1;
    }
    std::terminate();
}

auto calculate_offset(const BLBox& bounding_box,
                      const ScaledBaselineOffset& baseline_offsets,
                      HTextAlignment horizontal_alignment,
                      VTextAlignment vertical_alignment) -> BLPoint {
    return BLPoint {
        calculate_horizontal_offset(bounding_box, horizontal_alignment),
        calculate_vertical_offset(bounding_box, baseline_offsets, vertical_alignment),
    };
}

auto calculate_baseline_offset(FontStyle style [[maybe_unused]], const FontFace& face)
    -> BaselineOffset {
    const auto text =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    const auto font_size = float {16};

    const auto box =
        HarfbuzzShapedText {text, face.hb_font_face(), font_size}.bounding_box();

    using enum VTextAlignment;
    return BaselineOffset {
        .baseline_center = calculate_vertical_offset(box, {}, center) / font_size,
        .baseline_top = calculate_vertical_offset(box, {}, top) / font_size,
        .baseline_bottom = calculate_vertical_offset(box, {}, bottom) / font_size,
    };
}

}  // namespace

BaselineOffsets::BaselineOffsets(const FontFaces& faces) {
    for (auto& style : all_font_styles) {
        set(style, calculate_baseline_offset(style, faces.get(style)));
    }
}

auto BaselineOffsets::format() const -> std::string {
    const auto to_string = [this](FontStyle style) {
        return fmt::format("{} = {}", style, this->get(style));
    };
    const auto joined = fmt_join(",\n  ", all_font_styles, "{}", to_string);

    return fmt::format("BaselineOffsets(\n  {})\n", joined);
}

auto BaselineOffsets::get(FontStyle style, float font_size) const
    -> ScaledBaselineOffset {
    return get(style) * font_size;
}

auto BaselineOffsets::get(FontStyle style) const -> const BaselineOffset& {
    return ::logicsim::get<const BaselineOffset&>(*this, style);
}

auto BaselineOffsets::set(FontStyle style, BaselineOffset offset) -> void {
    ::logicsim::set(*this, style, std::move(offset));
}

//
// GlyphCache
//

GlyphCache::GlyphCache() : GlyphCache(get_default_font_locations()) {}

GlyphCache::GlyphCache(font_locations_t font_files)
    : font_faces_ {font_files}, baseline_offsets_ {font_faces_}, fonts_ {font_faces_} {}

auto GlyphCache::format() const -> std::string {
    return fmt::format("GlyphCache({} glyphs)", glyph_map_.size());
}

auto GlyphCache::clear() -> void {
    glyph_map_.clear();
}

auto GlyphCache::shrink_to_fit() -> void {
    glyph_map_.rehash(glyph_map_.size());
}

auto GlyphCache::get_font(float font_size, FontStyle style) const -> const BLFont& {
    // reuse font, to avoid allocation everytime we draw a text
    auto& font = fonts_.get(style);
    font.setSize(font_size);
    return font;
}

auto GlyphCache::calculate_bounding_box(std::string_view text, float font_size,
                                        FontStyle style) const -> BLBox {
    return HarfbuzzShapedText {text, font_faces_.get(style).hb_font_face(), font_size}
        .bounding_box();
}

auto GlyphCache::get_entry(std::string_view text, float font_size, FontStyle style,
                           HTextAlignment horizontal_alignment,
                           VTextAlignment vertical_alignment) const
    -> const glyph_entry_t& {
    const auto [it, inserted] = glyph_map_.try_emplace(glyph_key_t {
        .text_hash = wyhash(text),
        .font_size = font_size,
        .style = style,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
    });
    auto& entry = it->second;

    if (inserted) {
        const auto& font_face = font_faces_.get(style);

        entry.shaped_text =
            HarfbuzzShapedText {text, font_face.hb_font_face(), font_size};
        entry.offset = calculate_offset(entry.shaped_text.bounding_box(),
                                        baseline_offsets_.get(style, font_size),
                                        horizontal_alignment, vertical_alignment);
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
                           HTextAlignment horizontal_alignment,
                           VTextAlignment vertical_alignment, FontStyle style) const
    -> void {
    draw_text(ctx, position, text, font_size,
              TextAttributes {color, horizontal_alignment, vertical_alignment, style});
}

namespace {

auto text_width(const GlyphCache& glyph_cache, std::string_view text, FontStyle style)
    -> double {
    const auto font_size = float {16};
    const auto box = glyph_cache.calculate_bounding_box(text, font_size, style);
    return (box.x1 - box.x0) / font_size;
}

auto character_width(const GlyphCache& glyph_cache, char character, FontStyle style)
    -> double {
    const auto fill = '0';

    return text_width(glyph_cache, std::string {fill, character, fill}, style) -
           text_width(glyph_cache, std::string {fill, fill}, style);
}

}  // namespace

auto print_character_metrics(const GlyphCache& glyph_cache) -> void {
    for (const auto& style : all_font_styles) {
        print_fmt("{}:\n", style);

        for (const auto& character : "gJ0123456789,.-") {
            const auto width = character_width(glyph_cache, character, style);
            print_fmt("{}: {} grid\n", character, width);
        }

        print("");
    }
}

}  // namespace logicsim
