#include "render/font.h"

#include "file.h"
#include "font_style_property.h"
#include "iterator_adaptor/output_callable.h"
#include "logging.h"
#include "resource.h"

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

[[nodiscard]] auto create_hb_face(const std::string& data) -> HarfbuzzFontFace {
    return HarfbuzzFontFace {std::span<const char> {data.data(), data.size()}};
}

}  // namespace

auto load_face_or_warn(const std::filesystem::path& path) -> FontFace {
    const auto data = path.empty() ? "" : load_file(path);

    if (!path.empty() && data.empty()) {
        print("WARNING: could not open font file", path);
        return FontFace {};
    }

    return FontFace {
        .hb_font_face = create_hb_face(data),
        .bl_font_face = create_bl_face(data),
    };
}

Font::Font(const FontFace& font_face) : hb_font {font_face.hb_font_face}, bl_font {} {
    // doesn't matter, as we rescale them later
    constexpr auto create_font_size = float {10};
    bl_font.createFromFace(font_face.bl_font_face, create_font_size);
}

//
//
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

auto get_default_font_locations() -> font_locations_t {
    return font_locations_t {
        .regular = get_font_path(font_t::regular),
        .italic = get_font_path(font_t::italic),
        .bold = get_font_path(font_t::bold),
        .monospace = get_font_path(font_t::monospace),
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

Fonts::Fonts(const FontFaces& font_faces)
    : regular {Font {font_faces.regular}},
      italic {Font {font_faces.italic}},
      bold {Font {font_faces.bold}},
      monospace {Font {font_faces.monospace}} {}

auto Fonts::get(FontStyle style) const -> const Font& {
    return ::logicsim::get<const Font&>(*this, style);
}

auto Fonts::get(FontStyle style) -> Font& {
    return ::logicsim::get<Font&>(*this, style);
}

}  // namespace logicsim
