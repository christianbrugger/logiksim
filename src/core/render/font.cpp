#include "core/render/font.h"

#include "core/file.h"
#include "core/font_style_property.h"
#include "core/iterator_adaptor/output_callable.h"
#include "core/logging.h"
#include "core/resource.h"

#include <gsl/gsl>

#include <algorithm>

namespace logicsim {

namespace {

[[nodiscard]] auto to_bl_array(const std::string& data) -> BLArray<uint8_t> {
    auto array = BLArray<uint8_t> {};

    array.reserve(data.size());
    std::ranges::copy(data, output_callable([&](const char& c) { array.append(c); }));

    return array;
}

[[nodiscard]] auto to_bl_font_data(const std::string& data) -> BLFontData {
    const auto array = to_bl_array(data);

    auto font_data = BLFontData {};
    const auto status = font_data.createFromData(array);

    if (!font_data.empty() && status != BL_SUCCESS) [[unlikely]] {
        throw std::runtime_error("Could not create BLFontData");
    }

    return font_data;
}

[[nodiscard]] auto create_bl_face(const std::string& data) -> BLFontFace {
    const auto font_data = to_bl_font_data(data);

    auto face = BLFontFace {};
    const auto status = face.createFromData(font_data, 0);

    if (!data.empty() && status != BL_SUCCESS) [[unlikely]] {
        throw std::runtime_error("Could not create BLFontFace");
    }

    return face;
}

[[nodiscard]] auto create_hb_face(const std::string& data) -> HbFontFace {
    return HbFontFace {std::span<const char> {data.data(), data.size()}};
}

}  // namespace

FontFace::FontFace(const std::string& data)
    : hb_face_ {create_hb_face(data)}, bl_face_ {create_bl_face(data)} {
    Ensures(hb_face_.empty() == bl_face_.empty());
}

auto FontFace::empty() const -> bool {
    Expects(hb_face_.empty() == bl_face_.empty());

    return bl_face_.empty();
}

auto FontFace::hb_face() const -> const HbFontFace& {
    return hb_face_;
}

auto FontFace::bl_face() const -> const BLFontFace& {
    return bl_face_;
}

auto load_face_or_warn(const std::filesystem::path& path) -> FontFace {
    if (path.empty()) {
        return FontFace {};
    }

    const auto data = load_file(path);

    if (!data) {
        print("WARNING: could not open font file", path);
        return FontFace {};
    }

    auto face = FontFace {data.value()};

    if (face.empty()) {
        print("WARNING: font file resulted in an empty font face", path);
        return FontFace {};
    }

    return face;
}

namespace {

[[nodiscard]] auto create_bl_font(const BLFontFace& face, float font_size) -> BLFont {
    auto font = BLFont {};
    font.createFromFace(face, font_size);
    return font;
}

}  // namespace

Font::Font(const FontFace& face, float font_size)
    : hb_font_ {face.hb_face()}, bl_font_ {create_bl_font(face.bl_face(), font_size)} {
    Ensures(hb_font_.empty() == bl_font_.empty());
    Ensures(empty() == face.empty());
    Ensures(empty() || this->font_size() == font_size);
}

auto Font::hb_font() const -> const HbFont& {
    return hb_font_;
}

auto Font::bl_font() const -> const BLFont& {
    return bl_font_;
}

auto Font::empty() const -> bool {
    assert(hb_font_.empty() == bl_font_.empty());

    return bl_font_.empty();
}

auto Font::font_size() const -> float {
    return bl_font_.size();
}

auto Font::set_font_size(float font_size) -> void {
    bl_font_.setSize(font_size);

    assert(empty() || this->font_size() == font_size);
}

//
// Collections
//

auto font_locations_t::format() const -> std::string {
    return fmt::format(
        "font_locations_t{{\n"
        "    regular = {},\n"
        "    italic = {},\n"
        "    bold = {},\n"
        "    monospace = {}\n"
        "}}",
        regular, italic, bold, monospace);
}

auto font_locations_t::get(FontStyle style) const -> const std::filesystem::path& {
    return ::logicsim::get<const std::filesystem::path&>(*this, style);
}

auto get_default_font_location(FontStyle style) -> std::filesystem::path {
    return get_font_path(style);
}

auto get_default_font_locations() -> font_locations_t {
    return font_locations_t {
        .regular = get_default_font_location(FontStyle::regular),
        .italic = get_default_font_location(FontStyle::italic),
        .bold = get_default_font_location(FontStyle::bold),
        .monospace = get_default_font_location(FontStyle::monospace),
    };
}

FontFaces::FontFaces(const font_locations_t& font_files)
    : regular {load_face_or_warn(font_files.regular)},
      italic {load_face_or_warn(font_files.italic)},
      bold {load_face_or_warn(font_files.bold)},
      monospace {load_face_or_warn(font_files.monospace)} {}

auto FontFaces::get(FontStyle style) const -> const FontFace& {
    return ::logicsim::get<const FontFace&>(*this, style);
}

auto FontFaces::get(FontStyle style) -> const FontFace& {
    return ::logicsim::get<const FontFace&>(*this, style);
}

Fonts::Fonts(const FontFaces& font_faces, float font_size)
    : regular {Font {font_faces.regular, font_size}},
      italic {Font {font_faces.italic, font_size}},
      bold {Font {font_faces.bold, font_size}},
      monospace {Font {font_faces.monospace, font_size}} {}

auto Fonts::get(FontStyle style) const -> const Font& {
    return ::logicsim::get<const Font&>(*this, style);
}

auto Fonts::get(FontStyle style) -> Font& {
    return ::logicsim::get<Font&>(*this, style);
}

}  // namespace logicsim
