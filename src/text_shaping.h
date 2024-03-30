#ifndef LOGIKSIM_TEXT_SHAPING_H
#define LOGIKSIM_TEXT_SHAPING_H

#include "format/struct.h"

#include <blend2d.h>
#include <gsl/gsl>

#include <string>
#include <string_view>
#include <vector>

struct hb_blob_t;
struct hb_face_t;
struct hb_font_t;

namespace logicsim {

class HarfbuzzFontFace final {
   public:
    explicit HarfbuzzFontFace();
    explicit HarfbuzzFontFace(std::span<const char> font_data,
                              unsigned int font_index = 0);
    explicit HarfbuzzFontFace(const std::string &filename, unsigned int font_index = 0);
    ~HarfbuzzFontFace();

    HarfbuzzFontFace(const HarfbuzzFontFace &) = delete;
    HarfbuzzFontFace(HarfbuzzFontFace &&) = delete;
    HarfbuzzFontFace &operator=(const HarfbuzzFontFace &) = delete;
    HarfbuzzFontFace &operator=(HarfbuzzFontFace &&) = delete;

    [[nodiscard]] auto hb_face() const noexcept -> hb_face_t *;

   private:
    gsl::not_null<hb_face_t *> hb_face_;
};

class HarfbuzzFont final {
   public:
    explicit HarfbuzzFont();
    explicit HarfbuzzFont(const HarfbuzzFontFace &face, float font_size);
    ~HarfbuzzFont();

    HarfbuzzFont(const HarfbuzzFont &) = delete;
    HarfbuzzFont(HarfbuzzFont &&) = delete;
    HarfbuzzFont &operator=(const HarfbuzzFont &) = delete;
    HarfbuzzFont &operator=(HarfbuzzFont &&) = delete;

    [[nodiscard]] auto font_size() const noexcept -> float;
    [[nodiscard]] auto hb_font() const noexcept -> hb_font_t *;

   private:
    gsl::not_null<hb_font_t *> hb_font_;
    float font_size_ {};
};

class HarfbuzzShapedText {
   public:
    explicit HarfbuzzShapedText() = default;
    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFontFace &face,
                                float font_size);
    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFont &font);

    [[nodiscard]] auto operator==(const HarfbuzzShapedText &other) const -> bool = default;

    [[nodiscard]] auto glyph_run() const noexcept -> BLGlyphRun;
    [[nodiscard]] auto bounding_box() const noexcept -> BLBox;
    [[nodiscard]] auto bounding_rect() const noexcept -> BLRect;

    [[nodiscard]] auto format() const -> std::string;

   private:
    std::vector<uint32_t> codepoints_ {};
    std::vector<BLGlyphPlacement> placements_ {};

    BLBox bounding_box_ {};
};

}  // namespace logicsim

#endif