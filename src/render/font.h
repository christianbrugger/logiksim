#ifndef LOGICSIM_RENDER_FONT_H
#define LOGICSIM_RENDER_FONT_H

#include "format/struct.h"
#include "text_shaping.h"
#include "vocabulary/font_style.h"

#include <blend2d.h>

#include <filesystem>

namespace logicsim {

struct FontFace {
    HarfbuzzFontFace hb_font_face {};
    BLFontFace bl_font_face {};
};

[[nodiscard]] auto load_face_or_warn(const std::filesystem::path &font_file) -> FontFace;

static_assert(std::semiregular<FontFace>);

// TODO make fonts and faces private, as they depend on each other
// make function set_font_size

struct Font {
    HarfbuzzFont hb_font {};
    BLFont bl_font {};

    [[nodiscard]] explicit Font() = default;
    [[nodiscard]] explicit Font(const FontFace &font_face);
};

static_assert(std::semiregular<Font>);

//
//
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

[[nodiscard]] auto get_default_font_locations() -> font_locations_t;

struct FontFaces {
    FontFace regular {};
    FontFace italic {};
    FontFace bold {};
    FontFace monospace;

    [[nodiscard]] explicit FontFaces() = default;
    [[nodiscard]] explicit FontFaces(const font_locations_t &font_files);

    [[nodiscard]] auto get(FontStyle style) const -> const FontFace &;
};

static_assert(std::semiregular<FontFaces>);

struct Fonts {
    Font regular {};
    Font italic {};
    Font bold {};
    Font monospace;

    [[nodiscard]] explicit Fonts() = default;
    [[nodiscard]] explicit Fonts(const FontFaces &font_faces);

    [[nodiscard]] auto get(FontStyle style) const -> const Font &;
    [[nodiscard]] auto get(FontStyle style) -> Font &;
};

static_assert(std::semiregular<Fonts>);

}  // namespace logicsim

#endif
