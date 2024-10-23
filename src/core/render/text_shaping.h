#ifndef LOGIKSIM_RENDER_TEXT_SHAPING_H
#define LOGIKSIM_RENDER_TEXT_SHAPING_H

#include "core/format/struct.h"
#include "core/render/bl_glyph_placement.h"

#include <blend2d.h>

#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct hb_buffer_t;
struct hb_face_t;
struct hb_font_t;

using hb_codepoint_t = uint32_t;
struct hb_glyph_extents_t;
struct hb_glyph_info_t;
struct hb_glyph_position_t;

namespace logicsim {

class HbFontFace final {
   public:
    explicit HbFontFace();
    explicit HbFontFace(std::span<const char> font_data, unsigned int font_index = 0);

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto hb_face() const noexcept -> hb_face_t *;

   private:
    // immutable preserves whole parts relationship
    std::shared_ptr<hb_face_t> face_;
};

static_assert(std::semiregular<HbFontFace>);

class HbFont final {
   public:
    explicit HbFont();
    explicit HbFont(const HbFontFace &face);

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto hb_font() const noexcept -> hb_font_t *;

    [[nodiscard]] auto hb_glyph_extents(hb_codepoint_t codepoint) const
        -> std::optional<hb_glyph_extents_t>;
    [[nodiscard]] auto user_scale(float font_size) const -> BLPoint;

   private:
    // immutable preserves whole parts relationship
    std::shared_ptr<hb_font_t> font_;
};

static_assert(std::semiregular<HbFont>);

class HbShapedText {
   public:
    explicit HbShapedText();
    explicit HbShapedText(std::string_view text_utf8, HbFont font, float font_size);

    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto font() const -> const HbFont &;
    [[nodiscard]] auto font_size() const -> float;
    [[nodiscard]] auto user_scale() const -> BLPoint;

    [[nodiscard]] auto hb_buffer() const noexcept -> hb_buffer_t *;
    [[nodiscard]] auto hb_glyph_infos() const -> std::span<const hb_glyph_info_t>;
    [[nodiscard]] auto hb_glyph_positions() const -> std::span<const hb_glyph_position_t>;

   private:
    HbFont font_;
    float font_size_;

    // immutable preserves whole parts relationship
    std::shared_ptr<hb_buffer_t> buffer_;
};

static_assert(std::semiregular<HbShapedText>);

//
// Methods & Vocabulary
//

class GlyphPositionsDesign {
   public:
    explicit GlyphPositionsDesign() = default;
    explicit GlyphPositionsDesign(const HbShapedText &shaped_text);

    [[nodiscard]] auto operator==(const GlyphPositionsDesign &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    auto resize(std::size_t count) -> void;
    auto shrink_to_fit() -> void;

    [[nodiscard]] auto span() const -> std::span<const BLPoint>;

   private:
    std::vector<BLPoint> positions_ {};
};

static_assert(std::regular<GlyphPositionsDesign>);

class GlyphBoxesUser {
   public:
    explicit GlyphBoxesUser() = default;
    explicit GlyphBoxesUser(const HbShapedText &shaped_text,
                            const GlyphPositionsDesign &positions);

    [[nodiscard]] auto operator==(const GlyphBoxesUser &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    auto resize(std::size_t count) -> void;
    auto shrink_to_fit() -> void;

    [[nodiscard]] auto span() const -> std::span<const BLBox>;

   private:
    std::vector<BLBox> glyph_boxes_ {};
};

static_assert(std::regular<GlyphBoxesUser>);

/**
 * @brief: Bounding boxes of each graphene cluster
 */
struct ClusterBox {
    // first and last glyph index of the cluster
    std::size_t begin_index;  // inclusive
    std::size_t end_index;    // exclusive

    // bounding box of the cluster
    BLBox box;

    [[nodiscard]] auto operator==(const ClusterBox &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<ClusterBox>);

class ClusterBoxesUser {
   public:
    explicit ClusterBoxesUser() = default;
    explicit ClusterBoxesUser(const HbShapedText &shaped_text,
                              const GlyphBoxesUser &glyph_boxes);

    [[nodiscard]] auto operator==(const ClusterBoxesUser &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    auto resize(std::size_t count) -> void;
    auto shrink_to_fit() -> void;

    [[nodiscard]] auto span() const -> std::span<const ClusterBox>;

   private:
    std::vector<ClusterBox> cluster_boxes_ {};
};

static_assert(std::regular<ClusterBoxesUser>);

[[nodiscard]] auto get_codepoints(const HbShapedText &shaped_text)
    -> std::vector<uint32_t>;

[[nodiscard]] auto calculate_bounding_box_user(const GlyphBoxesUser &glyph_boxes)
    -> BLBox;

[[nodiscard]] auto calculate_bounding_box_user(std::string_view text_utf8, HbFont font,
                                               float font_size) -> BLBox;

struct glyph_count_result_t {
    std::size_t glyph_count;
    std::size_t cluster_count;

    [[nodiscard]] auto operator==(const glyph_count_result_t &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<glyph_count_result_t>);

[[nodiscard]] auto calculate_max_glyph_count(
    const ClusterBoxesUser &cluster_boxes, double max_text_width) -> glyph_count_result_t;

struct GlyphGeometryData {
   public:
    explicit GlyphGeometryData() = default;
    explicit GlyphGeometryData(const HbShapedText &shaped_text);
    explicit GlyphGeometryData(const HbShapedText &shaped_text, double max_text_width);

    [[nodiscard]] auto operator==(const GlyphGeometryData &other) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    [[nodiscard]] auto codepoints() const -> std::span<const uint32_t>;
    [[nodiscard]] auto positions() const -> const GlyphPositionsDesign &;
    [[nodiscard]] auto glyph_boxes() const -> const std::optional<GlyphBoxesUser> &;
    [[nodiscard]] auto cluster_boxes() const -> const std::optional<ClusterBoxesUser> &;
    [[nodiscard]] auto is_truncated() const -> bool;

    auto clear_glyph_boxes() -> void;
    auto clear_cluster_boxes() -> void;

   private:
    std::vector<uint32_t> codepoints_ {};
    GlyphPositionsDesign positions_ {};
    std::optional<GlyphBoxesUser> glyph_boxes_ {};
    std::optional<ClusterBoxesUser> cluster_boxes_ {};
    bool is_truncated_ {};
};

class HbGlyphRun {
   public:
    explicit HbGlyphRun() = default;
    explicit HbGlyphRun(const HbShapedText &shaped_text);
    explicit HbGlyphRun(const HbShapedText &shaped_text, double max_text_width);

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const HbGlyphRun &other) const -> bool = default;

    [[nodiscard]] auto glyph_run() const noexcept -> BLGlyphRun;
    [[nodiscard]] auto bounding_box() const noexcept -> BLBox;
    [[nodiscard]] auto bounding_rect() const noexcept -> BLRect;
    [[nodiscard]] auto is_truncated() const -> bool;

    [[nodiscard]] auto glyph_bounding_boxes() const
        -> const std::optional<GlyphBoxesUser> &;
    [[nodiscard]] auto cluster_bounding_boxes() const
        -> const std::optional<ClusterBoxesUser> &;

   private:
    GlyphGeometryData data_ {};
    BLBox bounding_box_ {};
};

static_assert(std::regular<HbGlyphRun>);

}  // namespace logicsim

#endif
