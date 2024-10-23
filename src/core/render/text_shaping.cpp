#include "core/render/text_shaping.h"

#include "core/algorithm/range.h"
#include "core/algorithm/round.h"
#include "core/algorithm/transform_to_vector.h"
#include "core/concept/input_range.h"
#include "core/format/blend2d_type.h"
#include "core/format/container.h"
#include "core/logging.h"
#include "core/timer.h"

#include <blend2d.h>
#include <fmt/core.h>
#include <gsl/gsl>
#include <hb.h>
#include <range/v3/algorithm/adjacent_find.hpp>
#include <range/v3/algorithm/max_element.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view/chunk_by.hpp>
#include <range/v3/view/exclusive_scan.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/partial_sum.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip_with.hpp>

#include <numeric>

namespace logicsim {

namespace {

//
// Declarations
//

struct HbBlobDeleter {
    auto operator()(hb_blob_t *hb_blob) -> void;
};

struct HbFaceDeleter {
    auto operator()(hb_face_t *hb_face) -> void;
};

struct HbFontDeleter {
    auto operator()(hb_font_t *hb_font) -> void;
};

struct HbBufferDeleter {
    auto operator()(hb_buffer_t *hb_buffer) -> void;
};

using HbBlobPointer = std::unique_ptr<hb_blob_t, HbBlobDeleter>;
using HbFacePointer = std::unique_ptr<hb_face_t, HbFaceDeleter>;
using HbFontPointer = std::unique_ptr<hb_font_t, HbFontDeleter>;
using HbBufferPointer = std::unique_ptr<hb_buffer_t, HbBufferDeleter>;

//
// Definitions
//

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

[[nodiscard]] auto create_hb_blob(std::span<const char> font_data) -> HbBlobPointer {
    const auto *data = font_data.data();
    const auto length = gsl::narrow<unsigned int>(font_data.size());
    const auto mode = hb_memory_mode_t::HB_MEMORY_MODE_DUPLICATE;

    void *user_data = nullptr;
    hb_destroy_func_t destroy = nullptr;

    auto blob = HbBlobPointer {
        hb_blob_create(data, length, mode, user_data, destroy),
    };

    Expects(blob != nullptr);
    Expects(hb_blob_get_length(blob.get()) == length);

    return blob;
}

[[nodiscard]] auto create_immutable_face() -> HbFacePointer {
    auto face = HbFacePointer {hb_face_reference(hb_face_get_empty())};
    hb_face_make_immutable(face.get());
    return face;
}

[[nodiscard]] auto create_immutable_face(std::span<const char> font_data,
                                         unsigned int font_index) -> HbFacePointer {
    const auto blob = create_hb_blob(font_data);

    auto face = HbFacePointer {hb_face_create(blob.get(), font_index)};
    hb_face_make_immutable(face.get());

    return face;
}

[[nodiscard]] auto create_immutable_font() -> HbFontPointer {
    auto font = HbFontPointer {hb_font_reference(hb_font_get_empty())};
    hb_font_make_immutable(font.get());
    return font;
}

[[nodiscard]] auto create_immutable_font(hb_face_t *hb_face) -> HbFontPointer {
    Expects(hb_face);

    auto font = HbFontPointer {hb_font_create(hb_face)};
    hb_font_make_immutable(font.get());

    return font;
}

[[nodiscard]] auto shape_text(std::string_view text_utf8,
                              hb_font_t *hb_font) -> HbBufferPointer {
    Expects(hb_font != nullptr);

    // TODO set const
    auto buffer = HbBufferPointer {hb_buffer_create()};
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

constexpr static inline auto empty_bl_box = BLBox {
    +std::numeric_limits<double>::infinity(),
    +std::numeric_limits<double>::infinity(),
    -std::numeric_limits<double>::infinity(),
    -std::numeric_limits<double>::infinity(),
};

[[nodiscard]] auto get_font_user_scale(hb_font_t *hb_font, float font_size) -> BLPoint {
    Expects(hb_font != nullptr);

    auto scale = BLPointI {};
    hb_font_get_scale(hb_font, &scale.x, &scale.y);

    return BLPoint {
        static_cast<double>(font_size) / scale.x,
        -static_cast<double>(font_size) / scale.y,
    };
}

[[nodiscard]] auto calculate_glyph_positions_design(hb_buffer_t *hb_buffer)
    -> std::vector<BLPoint> {
    const auto glyph_positions = get_hb_glyph_positions(hb_buffer);

    const auto to_advance = [](const hb_glyph_position_t &value) {
        return BLPoint {
            static_cast<double>(value.x_advance),
            static_cast<double>(value.y_advance),
        };
    };
    const auto origins = ranges::views::transform(glyph_positions, to_advance)  //
                         | ranges::views::exclusive_scan(BLPoint {}, std::plus {});

    const auto add_glyph_offset = [](const BLPoint &origin,
                                     const hb_glyph_position_t &value) {
        return BLPoint {
            origin.x + value.x_offset,
            origin.y + value.y_offset,
        };
    };

    return ranges::views::zip_with(add_glyph_offset, origins, glyph_positions) |
           ranges::to<std::vector>;
}

}  // namespace

GlyphPositionsDesign::GlyphPositionsDesign(hb_buffer_t *hb_buffer)
    : positions_ {calculate_glyph_positions_design(hb_buffer)} {}

auto GlyphPositionsDesign::format() const -> std::string {
    return fmt::format("{}", positions_);
}

auto GlyphPositionsDesign::span() const -> std::span<const BLPoint> {
    return positions_;
}

namespace {

// TODO move somewhere else
[[nodiscard]] auto get_box_union(const BLBox &a, const BLBox &b) -> BLBox {
    return BLBox {
        std::min(a.x0, b.x0),
        std::min(a.y0, b.y0),
        std::max(a.x1, b.x1),
        std::max(a.y1, b.y1),
    };
}

// TODO move somewhere else
// [[nodiscard]] auto get_box_union(std::span<const BLBox> boxes) -> BLBox {
[[nodiscard]] auto get_box_union(input_range_of<BLBox> auto &&boxes) -> BLBox {
    if (boxes.size() == 0) {
        return empty_bl_box;
    }

    return ranges::accumulate(boxes, empty_bl_box, [](const BLBox &a, const BLBox &b) {
        return get_box_union(a, b);
    });
}

[[nodiscard]] auto get_glyph_extents(hb_font_t *hb_font, hb_codepoint_t codepoint)
    -> std::optional<hb_glyph_extents_t> {
    Expects(hb_font != nullptr);

    auto extents = hb_glyph_extents_t {};
    if (hb_font_get_glyph_extents(hb_font, codepoint, &extents) != 0) {
        return extents;
    }

    return std::nullopt;
}

[[nodiscard]] auto calculate_glyph_boxes_user(const GlyphPositionsDesign &positions,
                                              hb_buffer_t *hb_buffer, hb_font_t *hb_font,
                                              float font_size) -> std::vector<BLBox> {
    const auto user_scale = get_font_user_scale(hb_font, font_size);
    const auto glyph_infos = get_glyph_infos(hb_buffer);

    const auto info_to_box = [&](const BLPoint &position, const hb_glyph_info_t &info) {
        if (const auto extents = get_glyph_extents(hb_font, info.codepoint);
            extents.has_value() && (extents->width != 0 || extents->height != 0)) {
            assert(extents->width >= 0);
            assert(-extents->height >= 0);
            return user_scale * BLBox {
                                    position.x + extents->x_bearing,
                                    position.y + extents->y_bearing,
                                    position.x + extents->x_bearing + extents->width,
                                    position.y + extents->y_bearing + extents->height,
                                };
        }
        return empty_bl_box;
    };

    return ranges::views::zip_with(info_to_box, positions.span(), glyph_infos) |
           ranges::to<std::vector>;
}

class GlyphBoxesUser {
   public:
    explicit GlyphBoxesUser(const GlyphPositionsDesign &positions, hb_buffer_t *hb_buffer,
                            hb_font_t *hb_font, float font_size);

    [[nodiscard]] auto operator==(const GlyphBoxesUser &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto span() const -> std::span<const BLBox>;

   private:
    std::vector<BLBox> glyph_boxes_;
};

GlyphBoxesUser::GlyphBoxesUser(const GlyphPositionsDesign &positions,
                               hb_buffer_t *hb_buffer, hb_font_t *hb_font,
                               float font_size)
    : glyph_boxes_ {
          calculate_glyph_boxes_user(positions, hb_buffer, hb_font, font_size)} {}

auto GlyphBoxesUser::format() const -> std::string {
    return fmt::format("{}", glyph_boxes_);
}

auto GlyphBoxesUser::span() const -> std::span<const BLBox> {
    return glyph_boxes_;
}

[[nodiscard]] auto calculate_bounding_box_user(const GlyphBoxesUser &glyph_boxes)
    -> BLBox {
    return get_box_union(glyph_boxes.span());
}

struct ClusterBox {
    // first and last glyph index of the cluster
    std::size_t begin_index;  // inclusive
    std::size_t end_index;    // exclusive

    // bounding box of the cluster
    BLBox box;

    [[nodiscard]] auto operator==(const ClusterBox &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

auto ClusterBox::format() const -> std::string {
    return fmt::format("ClusterBox(first_index = {}, last_index = {}, box = {})",
                       begin_index, end_index, box);
}

struct GlyphBoxData {
    std::size_t glyph_index;
    uint32_t cluster;
    BLBox box;

    [[nodiscard]] auto operator==(const GlyphBoxData &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

auto GlyphBoxData::format() const -> std::string {
    return fmt::format("GlyphBoxData(glyph_index = {}, cluster = {}, box = {})",
                       glyph_index, cluster, box);
}

/**
 * @brief: calculate bounding boxes of each graphene cluster
 */
[[nodiscard]] auto calculate_cluster_boxes_user(const GlyphBoxesUser &glyph_boxes,
                                                hb_buffer_t *hb_buffer)
    -> std::vector<ClusterBox> {
    const auto glyph_infos = get_glyph_infos(hb_buffer);
    Expects(glyph_infos.size() == glyph_boxes.span().size());

    const auto to_glyph_data = [](const std::size_t index, const hb_glyph_info_t &a,
                                  const BLBox &box) {
        return GlyphBoxData {
            .glyph_index = index,
            .cluster = a.cluster,
            .box = box,
        };
    };

    const auto is_cluster_equal = [](const GlyphBoxData &a,
                                     const GlyphBoxData &b) -> bool {
        return a.cluster == b.cluster;
    };

    const auto to_box_union = [](input_range_of<GlyphBoxData> auto &&rng) {
        const auto to_box = [](const GlyphBoxData &a) { return a.box; };
        return ClusterBox {
            .begin_index = (*std::ranges::begin(rng)).glyph_index,
            .end_index = (*std::ranges::prev(std::ranges::end(rng))).glyph_index + 1,
            .box = get_box_union(rng | ranges::views::transform(to_box)),
        };
    };

    return ranges::views::zip_with(to_glyph_data, ranges::views::iota(0),  //
                                   glyph_infos, glyph_boxes.span())        //
           | ranges::views::chunk_by(is_cluster_equal)                     //
           | ranges::views::transform(to_box_union)                        //
           | ranges::to<std::vector>;
}

class ClusterBoxesUser {
   public:
    explicit ClusterBoxesUser(const GlyphBoxesUser &glyph_boxes, hb_buffer_t *hb_buffer);

    [[nodiscard]] auto operator==(const ClusterBoxesUser &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto span() const -> std::span<const ClusterBox>;

   private:
    std::vector<ClusterBox> cluster_boxes_;
};

ClusterBoxesUser::ClusterBoxesUser(const GlyphBoxesUser &glyph_boxes,
                                   hb_buffer_t *hb_buffer)
    : cluster_boxes_ {calculate_cluster_boxes_user(glyph_boxes, hb_buffer)} {}

auto ClusterBoxesUser::format() const -> std::string {
    return fmt::format("{}", cluster_boxes_);
}

auto ClusterBoxesUser::span() const -> std::span<const ClusterBox> {
    return cluster_boxes_;
}

[[nodiscard]] auto calculate_max_glyph_count(const ClusterBoxesUser &cluster_boxes,
                                             double max_text_width) -> std::size_t {
    const auto union_box_data = [](const ClusterBox &a, const ClusterBox &b) {
        return ClusterBox {
            .begin_index = std::min(a.begin_index, b.begin_index),
            .end_index = std::max(a.end_index, b.end_index),
            .box = get_box_union(a.box, b.box),
        };
    };

    const auto box_fitting = [&](const ClusterBox &a) -> bool {
        return (a.box.x1 - a.box.x0) <= max_text_width;
    };

    const auto fitting =
        ranges::views::partial_sum(cluster_boxes.span(), union_box_data)  //
        | ranges::views::take_while(box_fitting);

    const auto max = ranges::max_element(fitting, {}, &ClusterBox::end_index);
    return max != fitting.end() ? (*max).end_index : std::size_t {0};
}

}  // namespace

//
// Font Face
//

HbFontFace::HbFontFace() : face_ {create_immutable_face()} {
    Expects(face_ != nullptr);
    Ensures(hb_face_is_immutable(face_.get()));
}

HbFontFace::HbFontFace(std::span<const char> font_data, unsigned int font_index)
    : face_ {create_immutable_face(font_data, font_index)} {
    Ensures(face_ != nullptr);
    Ensures(hb_face_is_immutable(face_.get()));
}

auto HbFontFace::empty() const -> bool {
    return hb_face_get_glyph_count(hb_face()) == 0;
}

auto HbFontFace::hb_face() const noexcept -> hb_face_t * {
    Expects(face_ != nullptr);
    Ensures(hb_face_is_immutable(face_.get()));

    return face_.get();
}

//
// Font
//

HbFont::HbFont() : font_ {create_immutable_font()} {
    Ensures(font_ != nullptr);
    Ensures(hb_font_is_immutable(font_.get()));
}

HbFont::HbFont(const HbFontFace &face) : font_ {create_immutable_font(face.hb_face())} {
    Ensures(font_ != nullptr);
    Ensures(hb_font_is_immutable(font_.get()));
}

auto HbFont::empty() const -> bool {
    const auto *face = hb_font_get_face(hb_font());
    Expects(face != nullptr);

    return hb_face_get_glyph_count(face) == 0;
}

auto HbFont::hb_font() const noexcept -> hb_font_t * {
    Expects(font_ != nullptr);
    Ensures(hb_font_is_immutable(font_.get()));

    return font_.get();
}

//
// Shaped Text
//

HbShapedText::HbShapedText(std::string_view text_utf8, const HbFont &font,
                           float font_size, std::optional<double> max_text_width) {
    const auto buffer = shape_text(text_utf8, font.hb_font());

    codepoints_ = get_uint32_codepoints(buffer.get());
    positions_ = GlyphPositionsDesign {buffer.get()};

    // boxes
    const auto glyph_boxes =
        GlyphBoxesUser {positions_, buffer.get(), font.hb_font(), font_size};
    const auto cluster_boxes = ClusterBoxesUser {glyph_boxes, buffer.get()};

    // count
    const auto glyph_count =
        max_text_width ? calculate_max_glyph_count(cluster_boxes, *max_text_width)
                       : glyph_boxes.span().size();

    bounding_box_ = calculate_bounding_box_user(glyph_boxes);

    codepoints_.resize(glyph_count);

    // Ensures(codepoints_.size() == positions_.span().size());
}

auto HbShapedText::empty() const -> bool {
    // Expects(codepoints_.size() == placements_.size());

    // TODO use glyph_count_ ???

    return codepoints_.empty();
}

auto HbShapedText::glyph_run() const noexcept -> BLGlyphRun {
    // Expects(codepoints_.size() == placements_.size());

    auto result = BLGlyphRun {};

    result.size = codepoints_.size();
    result.setGlyphData(codepoints_.data());
    result.setPlacementData(positions_.span().data());
    result.placementType = BL_GLYPH_PLACEMENT_TYPE_DESIGN_UNITS;

    return result;
}

auto HbShapedText::bounding_box() const noexcept -> BLBox {
    return bounding_box_;
}

auto HbShapedText::bounding_rect() const noexcept -> BLRect {
    const auto box = bounding_box_;
    return BLRect {box.x0, box.y0, box.x1 - box.x0, box.y1 - box.y0};
}

auto HbShapedText::format() const -> std::string {
    return fmt::format("ShapedText(codepoints = {}, positions = {}, bounding_box = {})",
                       codepoints_, positions_, bounding_box_);
}

}  // namespace logicsim
