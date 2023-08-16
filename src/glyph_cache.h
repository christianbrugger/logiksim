#ifndef LOGIKSIM_GLYPH_CACHE_H
#define LOGIKSIM_GLYPH_CACHE_H

#include "format.h"
#include "text_shaping.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <blend2d.h>

#include <bit>
#include <string_view>

namespace logicsim {
enum class HorizontalAlignment : uint8_t {
    left,
    right,
    center,
};
enum class VerticalAlignment : uint8_t {
    baseline,
    center,
    top,
    bottom,
};

template <>
auto format(HorizontalAlignment alignment) -> std::string;

template <>
auto format(VerticalAlignment alignment) -> std::string;

}  // namespace logicsim

namespace logicsim::glyph_cache {

auto hash(std::string_view text) noexcept -> uint64_t;

struct glyph_key_t {
    uint64_t text_hash {};
    float font_size {};
    HorizontalAlignment horizontal_alignment;
    VerticalAlignment vertical_alignment;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const glyph_key_t &other) const -> bool = default;
};

struct glyph_entry_t {
    HarfbuzzShapedText shaped_text {};
    BLPoint offset {0., 0.};

    [[nodiscard]] auto format() const -> std::string;
};

}  // namespace logicsim::glyph_cache

template <>
struct ankerl::unordered_dense::hash<logicsim::glyph_cache::glyph_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::glyph_cache::glyph_key_t &obj) const noexcept -> uint64_t {
        const uint64_t numerics =
            (uint64_t {std::bit_cast<uint32_t>(obj.font_size)} << 32) +
            (static_cast<uint64_t>(obj.horizontal_alignment) << 16) +
            static_cast<uint64_t>(obj.vertical_alignment);

        const uint64_t v0 = ankerl::unordered_dense::hash<uint64_t> {}(numerics);
        return ankerl::unordered_dense::detail::wyhash::mix(v0, obj.text_hash);
    }
};

namespace logicsim {

namespace defaults {
// constexpr static inline auto font_file = "Roboto-Regular.ttf";
constexpr static inline auto font_file = "NotoSans-Regular.ttf";
}  // namespace defaults

class GlyphCache {
   private:
    using glyph_key_t = glyph_cache::glyph_key_t;
    using glyph_entry_t = glyph_cache::glyph_entry_t;

   public:
    explicit GlyphCache();
    explicit GlyphCache(const std::string &font_file);

    [[nodiscard]] auto format() const -> std::string;

    auto draw_text(
        BLContext &ctx, const BLPoint &position, std::string_view text, float font_size,
        color_t color = defaults::color_black,
        HorizontalAlignment horizontal_alignment = HorizontalAlignment::left,
        VerticalAlignment vertical_alignment = VerticalAlignment::baseline) const -> void;

   private:
    [[nodiscard]] auto get_font(float font_size) const -> const BLFont &;
    [[nodiscard]] auto get_entry(std::string_view text, float font_size,
                                 HorizontalAlignment horizontal_alignment,
                                 VerticalAlignment vertical_alignment) const
        -> const glyph_entry_t &;

   private:
    using font_map_t = ankerl::unordered_dense::map<float, BLFont>;
    using glyph_map_t = ankerl::unordered_dense::map<glyph_key_t, glyph_entry_t>;

    HarfbuzzFontFace hb_font_face_ {};
    BLFontFace bl_font_face_ {};  // TODO !!! remove

    mutable font_map_t font_map_ {};
    mutable glyph_map_t glyph_map_ {};
};

}  // namespace logicsim

#endif
