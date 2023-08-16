#include "text_shaping.h"

#include "exception.h"
#include "format.h"

#include <blend2d.h>
#include <gsl/gsl>
#include <hb.h>

//
// Reference Implementation:
//
// https://github.com/aam/skiaex/blob/master/app/main.cpp
// https://gist.github.com/Danielku15/63a6a58cbe8cf0ac61d5b16e53715fd9
//
// https://source.chromium.org/chromium/chromium/src/+/master:ui/gfx/harfbuzz_font_skia.cc
// https://chromium.googlesource.com/skia/+/chrome/m57/tools/SkShaper_harfbuzz.cpp
//

namespace logicsim {

namespace {
static constexpr int font_size_scale = 1 << 16;
}

//
// HarfbuzzFace
//

HarfbuzzFace::HarfbuzzFace(std::string filename__, unsigned int font_index)
    : filename_ {std::move(filename__)},
      hb_blob_ {hb_blob_create_from_file(filename_.c_str())},
      hb_face_ {hb_face_create(hb_blob_, font_index)} {
    if (hb_blob_ == hb_blob_get_empty()) [[unlikely]] {
        // TODO exception type
        throw_exception(fmt::format("Font not found {}", filename_).c_str());
    }

    // set read only
    hb_blob_make_immutable(hb_blob_);
    hb_face_make_immutable(hb_face_);
}

HarfbuzzFace::~HarfbuzzFace() {
    hb_face_destroy(hb_face_);
    hb_blob_destroy(hb_blob_);
}

auto HarfbuzzFace::hb_face() const noexcept -> hb_face_t * {
    return hb_face_;
}

//
// HarfbuzzFont
//

HarfbuzzFont::HarfbuzzFont(const HarfbuzzFace &face, float font_size)
    : hb_font_ {hb_font_create(face.hb_face())} {
    const auto scale = gsl::narrow<int>(std::ceil(font_size * font_size_scale));
    hb_font_set_scale(hb_font_, scale, scale);
    hb_font_make_immutable(hb_font_);
}

HarfbuzzFont::~HarfbuzzFont() {
    hb_font_destroy(hb_font_);
}

auto HarfbuzzFont::hb_font() const noexcept -> hb_font_t * {
    return hb_font_;
}

//
// Harfbuzz Buffer
//

class HarfbuzzBuffer {
   public:
    explicit HarfbuzzBuffer() : hb_buffer {hb_buffer_create()} {}

    ~HarfbuzzBuffer() {
        hb_buffer_destroy(hb_buffer);
    }

   public:
    gsl::not_null<hb_buffer_t *> hb_buffer;
};

//
// Harfbuzz Shaped Text
//

HarfbuzzShapedText::HarfbuzzShapedText(std::string_view text_utf8,
                                       const HarfbuzzFace &face, float font_size)
    : HarfbuzzShapedText {text_utf8, HarfbuzzFont {face, font_size}} {}

HarfbuzzShapedText::HarfbuzzShapedText(std::string_view text_utf8,
                                       const HarfbuzzFont &font) {
    const auto buffer = HarfbuzzBuffer {};
    const auto hb_buffer = buffer.hb_buffer;

    const auto text_length = gsl::narrow<int>(text_utf8.size());
    const auto item_offset = (unsigned int) {0};
    const auto item_length = text_length;
    hb_buffer_add_utf8(hb_buffer, text_utf8.data(), text_length, item_offset,
                       item_length);

    // set text properties
    hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));
    hb_buffer_guess_segment_properties(hb_buffer);

    // shape text
    const hb_feature_t *features = nullptr;
    const auto num_features = (unsigned int) {0};
    hb_shape(font.hb_font(), hb_buffer, features, num_features);

    // extract placement data
    const auto glyph_count = hb_buffer_get_length(hb_buffer);
    const auto glyph_infos = std::span<hb_glyph_info_t>(
        hb_buffer_get_glyph_infos(hb_buffer, nullptr), glyph_count);
    const auto glyph_positions = std::span<hb_glyph_position_t>(
        hb_buffer_get_glyph_positions(hb_buffer, nullptr), glyph_count);

    codepoints_.reserve(glyph_count);
    placements_.reserve(glyph_count);

    std::ranges::transform(
        glyph_infos, std::back_inserter(codepoints_),
        [](const hb_glyph_info_t &glyph_info) { return glyph_info.codepoint; });

    std::ranges::transform(
        glyph_positions, std::back_inserter(placements_),
        [](const hb_glyph_position_t &position) {
            return BLGlyphPlacement {
                .placement = BLPointI {position.x_offset / font_size_scale,
                                       position.y_offset / font_size_scale},
                .advance = BLPointI {position.x_advance / font_size_scale,
                                     position.y_advance / font_size_scale},
            };
        });
}

auto HarfbuzzShapedText::glyph_run() -> BLGlyphRun {
    if (codepoints_.size() != placements_.size()) [[unlikely]] {
        throw_exception("data vectors need to have same size");
    }

    auto result = BLGlyphRun {};
    result.size = codepoints_.size();
    result.setGlyphData(codepoints_.data());
    result.setPlacementData(placements_.data());
    result.placementType = BL_GLYPH_PLACEMENT_TYPE_ADVANCE_OFFSET;

    return result;
}

//
// free functions
//

auto test_hb() -> void {
    const auto filename = "NotoSans-Regular.ttf";
    const auto text_utf8 = "test Q\u0305";
    const auto font_size = float {10};

    const auto face = HarfbuzzFace {filename};
    const auto font = HarfbuzzFont {face, font_size};
    const auto text = HarfbuzzShapedText {text_utf8, font};
}

}  // namespace logicsim
