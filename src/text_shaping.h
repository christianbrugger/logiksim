#ifndef LOGIKSIM_TEXT_SHAPING_H
#define LOGIKSIM_TEXT_SHAPING_H

#include "format/struct.h"

#include <blend2d.h>
#include <gsl/gsl>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct hb_blob_t;
struct hb_face_t;
struct hb_font_t;
struct hb_buffer_t;

namespace logicsim {

namespace detail {

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

}  // namespace detail

class HarfbuzzFontFace final {
   public:
    explicit HarfbuzzFontFace();
    explicit HarfbuzzFontFace(std::span<const char> font_data,
                              unsigned int font_index = 0);

    [[nodiscard]] auto hb_face() const noexcept -> hb_face_t *;

   private:
    // read-only, preserving whole parts relationship
    std::shared_ptr<hb_face_t> face_;
};

static_assert(std::semiregular<HarfbuzzFontFace>);

class HarfbuzzFont final {
   public:
    explicit HarfbuzzFont();
    explicit HarfbuzzFont(const HarfbuzzFontFace &face);

    [[nodiscard]] auto hb_font() const noexcept -> hb_font_t *;

   private:
    // read-only, preserving whole parts relationship
    std::shared_ptr<hb_font_t> font_;
};

static_assert(std::semiregular<HarfbuzzFont>);

class HarfbuzzShapedText {
   public:
    explicit HarfbuzzShapedText() = default;
    explicit HarfbuzzShapedText(std::string_view text_utf8, const HarfbuzzFont &font,
                                float font_size);

    [[nodiscard]] auto operator==(const HarfbuzzShapedText &other) const
        -> bool = default;

    [[nodiscard]] auto glyph_run() const noexcept -> BLGlyphRun;
    [[nodiscard]] auto bounding_box() const noexcept -> BLBox;
    [[nodiscard]] auto bounding_rect() const noexcept -> BLRect;

    [[nodiscard]] auto format() const -> std::string;

   private:
    std::vector<uint32_t> codepoints_ {};
    std::vector<BLGlyphPlacement> placements_ {};
    BLBox bounding_box_ {};
};

static_assert(std::regular<HarfbuzzShapedText>);

}  // namespace logicsim

#endif