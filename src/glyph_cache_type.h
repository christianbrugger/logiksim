#ifndef LOGIKSIM_GLYPH_CACHE_TYPE_H
#define LOGIKSIM_GLYPH_CACHE_TYPE_H

#include "format/enum.h"
#include "type_trait.h"
#include "vocabulary.h"

#include <exception>
#include <string_view>
#include <type_traits>

class BLContext;
struct BLPoint;

namespace logicsim {

enum class FontStyle : uint8_t {
    regular,
    italic,
    bold,
    monospace,
};

constexpr auto all_font_styles = {
    FontStyle::regular,
    FontStyle::italic,
    FontStyle::bold,
    FontStyle::monospace,
};

template <>
auto format(FontStyle style) -> std::string;

enum class HorizontalAlignment : uint8_t {
    left,
    right,
    center,
};

template <>
auto format(HorizontalAlignment alignment) -> std::string;

enum class VerticalAlignment : uint8_t {
    baseline,
    // adjusts the baseline
    center_baseline,
    top_baseline,
    bottom_baseline,
    // aligns the specific text
    center,
    top,
    bottom,
};

template <>
auto format(VerticalAlignment alignment) -> std::string;

// for style collections, i.e. types that store a value for each style
template <typename ReturnType, typename T>
auto get(T& obj, FontStyle style) -> ReturnType {
    switch (style) {
        using enum FontStyle;

        case regular:
            return obj.regular;
        case italic:
            return obj.italic;
        case bold:
            return obj.bold;
        case monospace:
            return obj.monospace;
    }
    std::terminate();
}

// for style collections, i.e. types that store a value for each style
template <typename T, typename V>
auto set(T& obj, FontStyle style, V&& value) -> void {
    switch (style) {
        using enum FontStyle;

        case regular:
            obj.regular = std::move(value);
            return;
        case italic:
            obj.italic = std::move(value);
            return;
        case bold:
            obj.bold = std::move(value);
            return;
        case monospace:
            obj.monospace = std::move(value);
            return;
    }
    std::terminate();
}

}  // namespace logicsim

#endif