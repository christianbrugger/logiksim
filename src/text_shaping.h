#ifndef LOGIKSIM_TEXT_SHAPING_H
#define LOGIKSIM_TEXT_SHAPING_H

#include <blend2d.h>

#include <string>
#include <string_view>

struct hb_blob_t;
struct hb_face_t;
struct hb_font_t;

namespace logicsim {

class HarfbuzzFace final {
   public:
    explicit HarfbuzzFace(std::string filename, unsigned int font_index = 0);
    ~HarfbuzzFace();

    auto hb_face() const noexcept -> hb_face_t *;

   private:
    std::string filename_ {};

    gsl::not_null<hb_blob_t *> hb_blob_;
    gsl::not_null<hb_face_t *> hb_face_;
};

class HarfbuzzFont final {
   public:
    explicit HarfbuzzFont(const HarfbuzzFace &face, float font_size);
    ~HarfbuzzFont();

    auto hb_font() const noexcept -> hb_font_t *;

   private:
    gsl::not_null<hb_font_t *> hb_font_;
};

class HarfbuzzShapedText {
   public:
    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFace &face,
                                float font_size);

    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFont &font);

    inline auto glyph_run() -> BLGlyphRun;

   private:
    std::vector<uint32_t> codepoints_ {};
    std::vector<BLGlyphPlacement> placements_ {};
};

auto test_hb() -> void;

}  // namespace logicsim

#endif