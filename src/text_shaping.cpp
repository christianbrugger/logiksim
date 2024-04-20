#include "text_shaping.h"

#include "algorithm/range.h"
#include "algorithm/round.h"
#include "algorithm/transform_to_vector.h"
#include "format/blend2d_type.h"
#include "format/container.h"

#include <blend2d.h>
#include <fmt/core.h>
#include <gsl/gsl>
#include <hb.h>

#include <numeric>

namespace logicsim {

namespace detail {

auto HbBlobDeleter::operator()(hb_blob_t *hb_blob) -> void {
    hb_blob_destroy(hb_blob);
}

auto HbFaceDeleter::operator()(hb_face_t *hb_face) -> void {
    hb_face_destroy(hb_face);
}

auto HbFontDeleter::operator()(hb_font_t *hb_font) -> void {
    hb_font_destroy(hb_font);
}

auto HbBufferDeleter::operator()(hb_buffer_t *hb_buffer) -> void {
    hb_buffer_destroy(hb_buffer);
}

}  // namespace detail

namespace {

[[nodiscard]] auto create_hb_blob(std::span<const char> font_data)
    -> detail::HbBlobPointer {
    const auto *data = font_data.data();
    const auto length = gsl::narrow<unsigned int>(font_data.size());
    const auto mode = hb_memory_mode_t::HB_MEMORY_MODE_DUPLICATE;

    void *user_data = nullptr;
    hb_destroy_func_t destroy = nullptr;

    // TODO make const
    auto blob =
        detail::HbBlobPointer {hb_blob_create(data, length, mode, user_data, destroy)};

    Expects(blob != nullptr);
    Expects(hb_blob_get_length(blob.get()) == length);

    return blob;
}

[[nodiscard]] auto create_immutable_face(std::span<const char> font_data,
                                         unsigned int font_index)
    -> detail::HbFacePointer {
    const auto blob = create_hb_blob(font_data);

    // TODO make const
    auto face = detail::HbFacePointer {hb_face_create(blob.get(), font_index)};
    hb_face_make_immutable(face.get());

    return face;
}

[[nodiscard]] auto create_immutable_font(hb_face_t *hb_face, float font_size)
    -> detail::HbFontPointer {
    Expects(hb_face);

    auto font = detail::HbFontPointer {hb_font_create(hb_face)};
    hb_font_set_ppem(font.get(), clamp_to<unsigned int>(font_size),
                     clamp_to<unsigned int>(font_size));
    hb_font_make_immutable(font.get());

    return font;
}

[[nodiscard]] auto shape_text(std::string_view text_utf8, hb_font_t *hb_font)
    -> detail::HbBufferPointer {
    Expects(hb_font != nullptr);

    // TODO set const
    auto buffer = detail::HbBufferPointer {hb_buffer_create()};
    Expects(buffer != nullptr);

    const auto text_length = gsl::narrow<int>(text_utf8.size());
    const auto item_offset = std::size_t {0};
    const auto item_length = text_length;
    hb_buffer_add_utf8(buffer.get(), text_utf8.data(), text_length, item_offset,
                       item_length);

    // set text properties
    hb_buffer_set_direction(buffer.get(), HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer.get(), HB_SCRIPT_LATIN);
    hb_buffer_set_language(buffer.get(), hb_language_from_string("en", -1));
    hb_buffer_guess_segment_properties(buffer.get());

    // shape text
    const hb_feature_t *features = nullptr;
    const auto num_features = std::size_t {0};
    hb_shape(hb_font, buffer.get(), features, num_features);

    return buffer;
}

[[nodiscard]] auto get_glyph_infos(hb_buffer_t *hb_buffer) -> std::span<hb_glyph_info_t> {
    Expects(hb_buffer != nullptr);

    const auto glyph_count = hb_buffer_get_length(hb_buffer);
    return std::span<hb_glyph_info_t>(hb_buffer_get_glyph_infos(hb_buffer, nullptr),
                                      glyph_count);
}

[[nodiscard]] auto get_hb_glyph_positions(hb_buffer_t *hb_buffer)
    -> std::span<hb_glyph_position_t> {
    Expects(hb_buffer != nullptr);

    const auto glyph_count = hb_buffer_get_length(hb_buffer);
    return std::span<hb_glyph_position_t>(
        hb_buffer_get_glyph_positions(hb_buffer, nullptr), glyph_count);
}

[[nodiscard]] auto get_uint32_codepoints(hb_buffer_t *hb_buffer)
    -> std::vector<uint32_t> {
    Expects(hb_buffer != nullptr);
    const auto glyph_infos = get_glyph_infos(hb_buffer);

    return transform_to_vector(glyph_infos, [](const hb_glyph_info_t &glyph_info) {
        return glyph_info.codepoint;
    });
}

[[nodiscard]] auto get_bl_placements(hb_buffer_t *hb_buffer)
    -> std::vector<BLGlyphPlacement> {
    Expects(hb_buffer != nullptr);
    const auto glyph_positions = get_hb_glyph_positions(hb_buffer);

    return transform_to_vector(glyph_positions, [](const hb_glyph_position_t &position) {
        return BLGlyphPlacement {
            .placement = BLPointI {position.x_offset, position.y_offset},
            .advance = BLPointI {position.x_advance, position.y_advance},
        };
    });
}

[[nodiscard]] auto calculate_bounding_rect(hb_buffer_t *hb_buffer, hb_font_t *hb_font,
                                           float font_size) -> BLBox {
    Expects(hb_buffer != nullptr);
    Expects(hb_font != nullptr);

    const auto glyph_infos = get_glyph_infos(hb_buffer);
    const auto glyph_positions = get_hb_glyph_positions(hb_buffer);

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

    for (auto i : range(std::min(glyph_infos.size(), glyph_positions.size()))) {
        const auto &pos = glyph_positions[i];
        auto extents = hb_glyph_extents_t {};

        if (hb_font_get_glyph_extents(hb_font, glyph_infos[i].codepoint, &extents) != 0 &&
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

    return rect / scale * font_size;
}

}  // namespace

//
// Harfbuzz Font Face
//

HarfbuzzFontFace::HarfbuzzFontFace() : face_ {hb_face_get_empty()} {}

HarfbuzzFontFace::HarfbuzzFontFace(std::span<const char> font_data,
                                   unsigned int font_index)
    : face_ {create_immutable_face(font_data, font_index)} {
    Ensures(face_ != nullptr);
}

auto HarfbuzzFontFace::hb_face() const noexcept -> hb_face_t * {
    Expects(face_ != nullptr);
    return face_.get();
}

//
// Harfbuzz Font
//

HarfbuzzFont::HarfbuzzFont() : font_ {hb_font_get_empty()} {}

HarfbuzzFont::HarfbuzzFont(const HarfbuzzFontFace &face, float font_size)
    : font_ {create_immutable_font(face.hb_face(), font_size)}, font_size_ {font_size} {
    Ensures(font_ != nullptr);
}

auto HarfbuzzFont::font_size() const noexcept -> float {
    return font_size_;
}

auto HarfbuzzFont::hb_font() const noexcept -> hb_font_t * {
    Expects(font_ != nullptr);
    return font_.get();
}

//
// Harfbuzz Shaped Text
//

HarfbuzzShapedText::HarfbuzzShapedText(std::string_view text_utf8,
                                       const HarfbuzzFontFace &face, float font_size)
    : HarfbuzzShapedText {text_utf8, HarfbuzzFont {face, font_size}} {}

HarfbuzzShapedText::HarfbuzzShapedText(std::string_view text_utf8,
                                       const HarfbuzzFont &font) {
    const auto buffer = shape_text(text_utf8, font.hb_font());

    codepoints_ = get_uint32_codepoints(buffer.get());
    placements_ = get_bl_placements(buffer.get());
    bounding_box_ =
        calculate_bounding_rect(buffer.get(), font.hb_font(), font.font_size());
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

auto HarfbuzzShapedText::format() const -> std::string {
    return fmt::format("ShapedText(codepoints = {}, placements = {}, bounding_box = {})",
                       codepoints_, placements_, bounding_box_);
}

}  // namespace logicsim
