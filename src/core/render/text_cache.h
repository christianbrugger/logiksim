#ifndef LOGIKSIM_RENDER_TEXT_CACHE_H
#define LOGIKSIM_RENDER_TEXT_CACHE_H

#include "core/format/struct.h"
#include "core/render/font.h"
#include "core/render/text_alignment.h"
#include "core/render/text_shaping.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/font_style.h"
#include "core/vocabulary/text_alignment.h"
#include "core/wyhash.h"

#include <ankerl/unordered_dense.h>
#include <blend2d.h>

#include <cassert>
#include <string>
#include <string_view>

namespace logicsim {

namespace text_cache {

struct cache_key_t {
    uint64_t text_hash;
    double max_text_width;  // -1 if not set
    float font_size;
    FontStyle style;
    HTextAlignment horizontal_alignment;
    VTextAlignment vertical_alignment;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const cache_key_t &other) const -> bool = default;
};

static_assert(std::regular<cache_key_t>);

struct cache_entry_t {
    HbGlyphRun hb_glyph_run {};
    BLPoint offset {0., 0.};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const cache_entry_t &other) const -> bool = default;
};

static_assert(std::regular<cache_entry_t>);

}  // namespace text_cache
}  // namespace logicsim

template <>
struct ankerl::unordered_dense::hash<logicsim::text_cache::cache_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::text_cache::cache_key_t &obj) const noexcept -> uint64_t {
        assert(obj.max_text_width == -1 || obj.max_text_width >= 0);

        const uint64_t numerics =
            (uint64_t {std::bit_cast<uint32_t>(obj.font_size)} << 32) +
            (static_cast<uint64_t>(obj.style) << 16) +
            (static_cast<uint64_t>(obj.horizontal_alignment) << 8) +
            (static_cast<uint64_t>(obj.vertical_alignment) << 0);

        const auto v1 = std::bit_cast<uint64_t>(obj.max_text_width);
        return logicsim::wyhash_192_bit(obj.text_hash, v1, numerics);
    }
};

namespace logicsim {

//
// Glyph Cache
//

class TextCache {
   private:
    using cache_key_t = text_cache::cache_key_t;
    using cache_entry_t = text_cache::cache_entry_t;

   public:
    explicit TextCache() = default;
    explicit TextCache(FontFaces faces);

    [[nodiscard]] auto format() const -> std::string;

    auto clear() const -> void;

    struct TextAttributes {
        color_t color {defaults::color_black};
        HTextAlignment horizontal_alignment {HTextAlignment::left};
        VTextAlignment vertical_alignment {VTextAlignment::baseline};
        FontStyle style {FontStyle::regular};

        // stop rendering characters when size limit is exceeded
        std::optional<double> max_text_width {std::nullopt};

        bool draw_bounding_rect {true};
        bool draw_glyph_rects {true};
        bool draw_cluster_rects {true};
    };

    auto draw_text(BLContext &ctx, const BLPoint &position, std::string_view text,
                   float font_size, TextAttributes attributes) const -> void;

    [[nodiscard]] auto calculate_bounding_box(std::string_view text, float font_size,
                                              FontStyle style) const -> BLBox;

   private:
    [[nodiscard]] auto get_scaled_bl_font(float font_size,
                                          FontStyle style) const -> const BLFont &;
    [[nodiscard]] auto get_entry(
        std::string_view text, float font_size, FontStyle style,
        HTextAlignment horizontal_alignment, VTextAlignment vertical_alignment,
        std::optional<double> max_text_width) const -> const cache_entry_t &;

   private:
    using glyph_map_t = ankerl::unordered_dense::map<cache_key_t, cache_entry_t>;

    FontFaces font_faces_ {};
    BaselineOffsets baseline_offsets_ {};

    mutable Fonts fonts_ {};
    mutable glyph_map_t glyph_map_ {};
};

static_assert(std::semiregular<TextCache>);

auto print_character_metrics(const TextCache &glyph_cache) -> void;

}  // namespace logicsim

#endif
