#ifndef LOGICSIM_RENDER_FONT_H
#define LOGICSIM_RENDER_FONT_H

#include "core/format/struct.h"
#include "core/render/text_shaping.h"
#include "core/vocabulary/font_style.h"

#include <blend2d.h>

#include <filesystem>

namespace logicsim {

/**
 * @brief: Harfbuzz and Blend2d font-face with the same data.
 */
class FontFace {
   public:
    explicit FontFace() = default;
    explicit FontFace(const std::string &data);

    [[nodiscard]] auto hb_face() const -> const HbFontFace &;
    [[nodiscard]] auto bl_face() const -> const BLFontFace &;

    [[nodiscard]] auto empty() const -> bool;

   private:
    HbFontFace hb_face_ {};
    BLFontFace bl_face_ {};
};

static_assert(std::semiregular<FontFace>);

[[nodiscard]] auto load_face_or_warn(const std::filesystem::path &path) -> FontFace;

/**
 * @brief: Harfbuzz and Blend2d font from the same data.
 */
class Font {
   public:
    [[nodiscard]] explicit Font() = default;
    [[nodiscard]] explicit Font(const FontFace &font_face, float font_size);

    [[nodiscard]] auto hb_font() const -> const HbFont &;
    [[nodiscard]] auto bl_font() const -> const BLFont &;

    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto font_size() const -> float;
    auto set_font_size(float font_size) -> void;

   private:
    HbFont hb_font_ {};
    BLFont bl_font_ {};
};

static_assert(std::semiregular<Font>);

//
// Collections
//

struct font_locations_t {
    std::filesystem::path regular;
    std::filesystem::path italic;
    std::filesystem::path bold;
    std::filesystem::path monospace;

    [[nodiscard]] auto get(FontStyle style) const -> const std::filesystem::path &;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const font_locations_t &) const -> bool = default;
};

static_assert(std::regular<font_locations_t>);

[[nodiscard]] auto get_default_font_location(FontStyle style) -> std::filesystem::path;
[[nodiscard]] auto get_default_font_locations() -> font_locations_t;

struct FontFaces {
    FontFace regular {};
    FontFace italic {};
    FontFace bold {};
    FontFace monospace;

    [[nodiscard]] explicit FontFaces() = default;
    [[nodiscard]] explicit FontFaces(const font_locations_t &font_files);

    [[nodiscard]] auto get(FontStyle style) const -> const FontFace &;
    [[nodiscard]] auto get(FontStyle style) -> const FontFace &;
};

static_assert(std::semiregular<FontFaces>);

struct Fonts {
    Font regular {};
    Font italic {};
    Font bold {};
    Font monospace;

    [[nodiscard]] explicit Fonts() = default;
    [[nodiscard]] explicit Fonts(const FontFaces &font_faces, float font_size);

    [[nodiscard]] auto get(FontStyle style) const -> const Font &;
    [[nodiscard]] auto get(FontStyle style) -> Font &;
};

static_assert(std::semiregular<Fonts>);

}  // namespace logicsim

#endif
