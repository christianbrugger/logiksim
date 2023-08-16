#include "text_shaping.h"

#include "exception.h"
#include "format.h"
#include "range.h"

#include <blend2d.h>
#include <gsl/gsl>
#include <hb.h>

#include <numeric>

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
static constexpr int font_size_scale = 1;  // 1 << 16;
}

//
// Harfbuzz Blob
//

class HarfbuzzBlob {
   public:
    explicit HarfbuzzBlob() : hb_blob {hb_blob_get_empty()} {}

    explicit HarfbuzzBlob(const std::string &filename)
        : hb_blob {hb_blob_create_from_file(filename.c_str())} {
        if (hb_blob == hb_blob_get_empty()) [[unlikely]] {
            // TODO exception type
            throw_exception(fmt::format("Font not found {}", filename).c_str());
        }
    }

    HarfbuzzBlob(const HarfbuzzBlob &) = delete;
    HarfbuzzBlob(HarfbuzzBlob &&) = delete;
    HarfbuzzBlob &operator=(const HarfbuzzBlob &) = delete;
    HarfbuzzBlob &operator=(HarfbuzzBlob &&) = delete;

    ~HarfbuzzBlob() {
        hb_blob_destroy(hb_blob);
    }

   public:
    gsl::not_null<hb_blob_t *> hb_blob;
};

//
// Harfbuzz Font Face
//

HarfbuzzFontFace::HarfbuzzFontFace() : hb_face_ {hb_face_get_empty()} {}

HarfbuzzFontFace::HarfbuzzFontFace(const std::string &filename, unsigned int font_index)
    : hb_face_ {hb_face_create(HarfbuzzBlob {filename.c_str()}.hb_blob, font_index)}

{
    hb_face_make_immutable(hb_face_);
}

HarfbuzzFontFace::~HarfbuzzFontFace() {
    hb_face_destroy(hb_face_);
}

auto HarfbuzzFontFace::hb_face() const noexcept -> hb_face_t * {
    return hb_face_;
}

//
// Harfbuzz Font
//

HarfbuzzFont::HarfbuzzFont() : hb_font_ {hb_font_get_empty()} {}

HarfbuzzFont::HarfbuzzFont(const HarfbuzzFontFace &face, float font_size)
    : hb_font_ {hb_font_create(face.hb_face())}, font_size_ {font_size} {
    hb_font_set_ppem(hb_font_, round_fast(font_size), round_fast(font_size));
    hb_font_make_immutable(hb_font_);
}

HarfbuzzFont::~HarfbuzzFont() {
    hb_font_destroy(hb_font_);
}

auto HarfbuzzFont::font_size() const noexcept -> float {
    return font_size_;
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

    HarfbuzzBuffer(const HarfbuzzBuffer &) = delete;
    HarfbuzzBuffer(HarfbuzzBuffer &&) = delete;
    HarfbuzzBuffer &operator=(const HarfbuzzBuffer &) = delete;
    HarfbuzzBuffer &operator=(HarfbuzzBuffer &&) = delete;

    ~HarfbuzzBuffer() {
        hb_buffer_destroy(hb_buffer);
    }

   public:
    gsl::not_null<hb_buffer_t *> hb_buffer;
};

//
// Harfbuzz Shaped Text
//

auto calculate_bounding_rect(std::span<const hb_glyph_info_t> glyph_info,
                             std::span<hb_glyph_position_t> glyph_positions,
                             const HarfbuzzFont &font) -> BLBox {
    const auto hb_font = font.hb_font();

    auto scale = BLPointI {};
    hb_font_get_scale(hb_font, &scale.x, &scale.y);

    auto origin = BLPoint {};
    auto rect = BLBox {
        +std::numeric_limits<double>::infinity(),
        +std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
    };
    bool found = false;

    for (auto i : range(std::min(glyph_info.size(), glyph_positions.size()))) {
        const auto &pos = glyph_positions[i];
        auto extents = hb_glyph_extents_t {};

        if (hb_font_get_glyph_extents(hb_font, glyph_info[i].codepoint, &extents) &&
            extents.width != 0 && extents.height != 0) {
            const auto glyph_rect = BLBox {
                origin.x + pos.x_offset + extents.x_bearing,
                -(origin.y + pos.y_offset + extents.y_bearing),
                origin.x + pos.x_offset + extents.x_bearing + extents.width,
                -(origin.y + pos.y_offset + extents.y_bearing + extents.height),
            };

            assert(glyph_rect.x0 <= glyph_rect.x1);
            assert(glyph_rect.y0 <= glyph_rect.y1);

            rect.x0 = std::min(rect.x0, glyph_rect.x0);
            rect.y0 = std::min(rect.y0, glyph_rect.y0);
            rect.x1 = std::max(rect.x1, glyph_rect.x1);
            rect.y1 = std::max(rect.y1, glyph_rect.y1);

            found = true;
        }

        origin.x += pos.x_advance;
        origin.y += pos.y_advance;
    }

    if (!found || scale.x == 0 || scale.y == 0) {
        return BLBox {};
    }

    return rect / scale * font.font_size();
}

HarfbuzzShapedText::HarfbuzzShapedText(std::string_view text_utf8,
                                       const HarfbuzzFontFace &face, float font_size)
    : HarfbuzzShapedText {text_utf8, HarfbuzzFont {face, font_size}} {}

HarfbuzzShapedText::HarfbuzzShapedText(std::string_view text_utf8,
                                       const HarfbuzzFont &font) {
    const auto buffer = HarfbuzzBuffer {};
    const auto hb_buffer = buffer.hb_buffer;

    const auto text_length = gsl::narrow<int>(text_utf8.size());
    const auto item_offset = std::size_t {0};
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
    const auto num_features = std::size_t {0};
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

    bounding_box_ = calculate_bounding_rect(glyph_infos, glyph_positions, font);
}

auto HarfbuzzShapedText::glyph_run() const noexcept -> BLGlyphRun {
    auto result = BLGlyphRun {};

    result.size = std::min(codepoints_.size(), placements_.size());
    result.setGlyphData(codepoints_.data());
    result.setPlacementData(placements_.data());
    result.placementType = BL_GLYPH_PLACEMENT_TYPE_ADVANCE_OFFSET;

    return result;
}

auto HarfbuzzShapedText::bounding_box() const noexcept -> BLBox {
    return bounding_box_;
}

auto HarfbuzzShapedText::bounding_rect() const noexcept -> BLRect {
    const auto box = bounding_box_;
    return BLRect {box.x0, box.y0, box.x1 - box.x0, box.y1 - box.y0};
}

}  // namespace logicsim
