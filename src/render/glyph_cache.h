#ifndef LOGIKSIM_RENDER_GLYPH_CACHE_H
#define LOGIKSIM_RENDER_GLYPH_CACHE_H

#include "format/struct.h"
#include "text_shaping.h"
#include "vocabulary/color.h"
#include "vocabulary/font_style.h"
#include "vocabulary/text_alignment.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>
#include <blend2d.h>

#include <bit>
#include <filesystem>
#include <string>
#include <string_view>

namespace logicsim {

struct font_locations_t {
    std::filesystem::path regular;
    std::filesystem::path italic;
    std::filesystem::path bold;
    std::filesystem::path monospace;

    [[nodiscard]] auto get(FontStyle style) const -> const std::filesystem::path &;
};

[[nodiscard]] auto get_default_font_locations() -> font_locations_t;

namespace glyph_cache {

struct glyph_key_t {
    uint64_t text_hash;
    float font_size;
    FontStyle style;
    HTextAlignment horizontal_alignment;
    VTextAlignment vertical_alignment;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const glyph_key_t &other) const -> bool = default;
};

struct glyph_entry_t {
    HarfbuzzShapedText shaped_text {};
    BLPoint offset {0., 0.};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const glyph_entry_t &other) const -> bool = default;
};

}  // namespace glyph_cache
}  // namespace logicsim

template <>
struct ankerl::unordered_dense::hash<logicsim::glyph_cache::glyph_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::glyph_cache::glyph_key_t &obj) const noexcept -> uint64_t {
        const uint64_t numerics =
            (uint64_t {std::bit_cast<uint32_t>(obj.font_size)} << 32) +
            (static_cast<uint64_t>(obj.style) << 16) +
            (static_cast<uint64_t>(obj.horizontal_alignment) << 8) +
            (static_cast<uint64_t>(obj.vertical_alignment) << 0);

        const uint64_t v0 = ankerl::unordered_dense::hash<uint64_t> {}(numerics);
        return logicsim::wyhash_128_bit(v0, obj.text_hash);
    }
};

namespace logicsim {

//
// Font Collections
//

struct FontFace {
   public:
    [[nodiscard]] explicit FontFace() = default;
    [[nodiscard]] explicit FontFace(const std::filesystem::path &font_file);

    [[nodiscard]] auto hb_font_face() const -> const HarfbuzzFontFace &;
    [[nodiscard]] auto bl_font_face() const -> const BLFontFace &;

   private:
    std::string font_data_ {};

    HarfbuzzFontFace hb_font_face_ {};

    BLFontData bl_font_data_ {};
    BLFontFace bl_font_face_ {};
};

struct FontFaces {
    FontFace regular {};
    FontFace italic {};
    FontFace bold {};
    FontFace monospace;

    [[nodiscard]] explicit FontFaces() = default;
    [[nodiscard]] explicit FontFaces(const font_locations_t &font_files);

    [[nodiscard]] auto get(FontStyle style) const -> const FontFace &;
};

struct Fonts {
    BLFont regular {};
    BLFont italic {};
    BLFont bold {};
    BLFont monospace;

    [[nodiscard]] explicit Fonts() = default;
    [[nodiscard]] explicit Fonts(const FontFaces &font_faces);

    [[nodiscard]] auto get(FontStyle style) const -> const BLFont &;
    [[nodiscard]] auto get(FontStyle style) -> BLFont &;
};

struct ScaledBaselineOffset;

// offsets for fontsize 1.0
struct BaselineOffset {
    double baseline_center {};
    double baseline_top {};
    double baseline_bottom {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator*(float font_size) const -> ScaledBaselineOffset;
    [[nodiscard]] constexpr auto operator==(const BaselineOffset &other) const
        -> bool = default;
};

// using strong type for scaled offsets for a specific font size
struct ScaledBaselineOffset {
    double baseline_center {};
    double baseline_top {};
    double baseline_bottom {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const ScaledBaselineOffset &other) const
        -> bool = default;
};

struct BaselineOffsets {
    BaselineOffset regular {};
    BaselineOffset italic {};
    BaselineOffset bold {};
    BaselineOffset monospace;

    [[nodiscard]] explicit BaselineOffsets(const FontFaces &faces);
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const BaselineOffsets &other) const
        -> bool = default;

    [[nodiscard]] auto get(FontStyle style, float font_size) const
        -> ScaledBaselineOffset;

   private:
    [[nodiscard]] auto get(FontStyle style) const -> const BaselineOffset &;
    auto set(FontStyle style, BaselineOffset offset) -> void;
};

//
// Glyph Cache
//

class GlyphCache {
   private:
    using glyph_key_t = glyph_cache::glyph_key_t;
    using glyph_entry_t = glyph_cache::glyph_entry_t;

   public:
    explicit GlyphCache();
    explicit GlyphCache(const font_locations_t &font_files);

    [[nodiscard]] auto format() const -> std::string;

    auto clear() -> void;
    auto shrink_to_fit() -> void;

    struct TextAttributes {
        color_t color {defaults::color_black};
        HTextAlignment horizontal_alignment {HTextAlignment::left};
        VTextAlignment vertical_alignment {VTextAlignment::baseline};
        FontStyle style {FontStyle::regular};
    };

    auto draw_text(BLContext &ctx, const BLPoint &position, std::string_view text,
                   float font_size, TextAttributes attributes) const -> void;

    auto draw_text(BLContext &ctx, const BLPoint &position, std::string_view text,
                   float font_size, color_t color = defaults::color_black,
                   HTextAlignment horizontal_alignment = HTextAlignment::left,
                   VTextAlignment vertical_alignment = VTextAlignment::baseline,
                   FontStyle style = FontStyle::regular) const -> void;

    [[nodiscard]] auto calculate_bounding_box(std::string_view text, float font_size,
                                              FontStyle style) const -> BLBox;

   private:
    [[nodiscard]] auto get_font(float font_size, FontStyle style) const -> const BLFont &;
    [[nodiscard]] auto get_entry(std::string_view text, float font_size, FontStyle style,
                                 HTextAlignment horizontal_alignment,
                                 VTextAlignment vertical_alignment) const
        -> const glyph_entry_t &;

   private:
    using glyph_map_t = ankerl::unordered_dense::map<glyph_key_t, glyph_entry_t>;

    FontFaces font_faces_;
    BaselineOffsets baseline_offsets_;

    mutable Fonts fonts_;
    mutable glyph_map_t glyph_map_ {};
};

auto print_character_metrics(const GlyphCache &glyph_cache) -> void;

}  // namespace logicsim

#endif
