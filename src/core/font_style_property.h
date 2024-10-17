#ifndef LOGICSIM_FONT_STYLE_PROPERTY_H
#define LOGICSIM_FONT_STYLE_PROPERTY_H

#include "core/vocabulary/font_style.h"

#include <exception>

namespace logicsim {

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
            obj.regular = std::forward<V>(value);
            return;
        case italic:
            obj.italic = std::forward<V>(value);
            return;
        case bold:
            obj.bold = std::forward<V>(value);
            return;
        case monospace:
            obj.monospace = std::forward<V>(value);
            return;
    }
    std::terminate();
}

}  // namespace logicsim

#endif
