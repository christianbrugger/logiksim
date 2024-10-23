#include "core/render/text_cache.h"

#include "core/logging.h"
#include "core/render/text_cache.h"
#include "core/timer.h"  // TODO remove

#include <fmt/core.h>
#include <gsl/gsl>

#include <exception>

namespace logicsim {

namespace text_cache {

auto cache_key_t::format() const -> std::string {
    return fmt::format("({}, {}, {}, {}, {})", text_hash, font_size, style,
                       horizontal_alignment, vertical_alignment);
}

auto cache_entry_t::format() const -> std::string {
    return fmt::format("({}, {})", offset.x, offset.y);
}

}  // namespace text_cache

//
// TextCache
//

namespace {
/**
 * @brief: initial size, fonts will be rescaled later
 */
constexpr auto CACHE_FONT_INITIAL_SIZE = float {10};
}  // namespace

TextCache::TextCache(FontFaces faces)
    : font_faces_ {std::move(faces)},
      baseline_offsets_ {font_faces_},
      fonts_ {font_faces_, CACHE_FONT_INITIAL_SIZE} {}

auto TextCache::format() const -> std::string {
    return fmt::format("TextCache({} glyphs)", glyph_map_.size());
}

auto TextCache::clear() const -> void {
    glyph_map_ = glyph_map_t {};

    Ensures(glyph_map_.values().size() == 0);
    Ensures(glyph_map_.values().capacity() == 0);
}

auto TextCache::get_scaled_bl_font(float font_size,
                                   FontStyle style) const -> const BLFont& {
    // reuse font to avoid allocation in every draw call
    auto& font = fonts_.get(style);
    font.set_font_size(font_size);
    return font.bl_font();
}

auto TextCache::calculate_bounding_box(std::string_view text, float font_size,
                                       FontStyle style) const -> BLBox {
    const auto& font = fonts_.get(style).hb_font();
    return calculate_bounding_box_user(text, font, font_size);
}

auto TextCache::get_entry(
    std::string_view text, float font_size, FontStyle style,
    HTextAlignment horizontal_alignment, VTextAlignment vertical_alignment,
    std::optional<double> max_text_width) const -> const cache_entry_t& {
    if (max_text_width && *max_text_width < 0) [[unlikely]] {
        throw std::runtime_error("max_text_width cannot be negative if set.");
    }

    // TODO don't add font-size and max-text-width to cache-key
    const auto [it, inserted] = glyph_map_.try_emplace(cache_key_t {
        .text_hash = wyhash(text),
        .max_text_width = max_text_width ? *max_text_width : -1,
        .font_size = font_size,
        .style = style,
        .horizontal_alignment = horizontal_alignment,
        .vertical_alignment = vertical_alignment,
    });
    auto& entry = it->second;

    if (inserted) {
        const auto& hb_font = fonts_.get(style).hb_font();
        const auto hb_shaped_text = HbShapedText {text, hb_font, font_size};

        entry.hb_glyph_run = max_text_width ? HbGlyphRun {hb_shaped_text, *max_text_width}
                                            : HbGlyphRun {hb_shaped_text};
        entry.offset = calculate_offset(entry.hb_glyph_run.bounding_box(),
                                        baseline_offsets_.get(style, font_size),
                                        horizontal_alignment, vertical_alignment);
    }

    return entry;
}

auto TextCache::draw_text(BLContext& ctx, const BLPoint& position, std::string_view text,
                          float font_size, TextAttributes attributes) const -> void {
    if (text.empty()) {
        return;
    }

    const auto& font = get_scaled_bl_font(font_size, attributes.style);
    const auto& entry = get_entry(text, font_size, attributes.style,  //
                                  attributes.horizontal_alignment,    //
                                  attributes.vertical_alignment,      //
                                  attributes.max_text_width);
    const auto origin = position - entry.offset;

    ctx.fillGlyphRun(origin, font, entry.hb_glyph_run.glyph_run(), attributes.color);

    if (attributes.draw_bounding_rect) {
        ctx.setStrokeWidth(1);
        ctx.translate(origin);
        ctx.strokeRect(entry.hb_glyph_run.bounding_rect(), defaults::color_lime);
        ctx.translate(-origin);
    }
    // if (attributes.draw_glyph_rects) {
    //     for (const auto rect : entry.shaped_text.glyph_bounding_rects()) {
    //         print(rect);
    //     }
    // }
}

namespace {

auto text_width(const TextCache& glyph_cache, std::string_view text,
                FontStyle style) -> double {
    const auto font_size = float {16};
    const auto box = glyph_cache.calculate_bounding_box(text, font_size, style);
    return (box.x1 - box.x0) / font_size;
}

auto character_width(const TextCache& glyph_cache, char character,
                     FontStyle style) -> double {
    const auto fill = '0';

    return text_width(glyph_cache, std::string {fill, character, fill}, style) -
           text_width(glyph_cache, std::string {fill, fill}, style);
}

}  // namespace

auto print_character_metrics(const TextCache& glyph_cache) -> void {
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
