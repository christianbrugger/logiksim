#ifndef LOGIKSIM_TEXT_SHAPING_H
#define LOGIKSIM_TEXT_SHAPING_H

#include <blend2d.h>

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
    explicit HarfbuzzFontFace(std::string filename, unsigned int font_index = 0);
    ~HarfbuzzFontFace();

    auto hb_face() const noexcept -> hb_face_t *;

   private:
    std::string filename_ {};

    gsl::not_null<hb_blob_t *> hb_blob_;
    gsl::not_null<hb_face_t *> hb_face_;
};

class HarfbuzzFont final {
   public:
    explicit HarfbuzzFont();
    explicit HarfbuzzFont(const HarfbuzzFontFace &face, float font_size);
    ~HarfbuzzFont();

    auto hb_font() const noexcept -> hb_font_t *;

   private:
    gsl::not_null<hb_font_t *> hb_font_;
};

class HarfbuzzShapedText {
   public:
    explicit HarfbuzzShapedText() = default;
    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFontFace &face,
                                float font_size);
    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFont &font);

    auto operator==(const HarfbuzzShapedText &other) const -> bool = default;

    auto glyph_run() const -> BLGlyphRun;

   private:
    std::vector<uint32_t> codepoints_ {};
    std::vector<BLGlyphPlacement> placements_ {};

   public:
};

}  // namespace logicsim

#endif