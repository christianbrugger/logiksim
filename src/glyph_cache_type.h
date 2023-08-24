#ifndef LOGIKSIM_GLYPH_CACHE_TYPE_H
#define LOGIKSIM_GLYPH_CACHE_TYPE_H

#include "format.h"
#include "type_trait.h"
#include "vocabulary.h"

#include <string_view>
#include <type_traits>

class BLContext;
struct BLPoint;

namespace logicsim {

enum class FontStyle : uint8_t {
    regular,
    italic,
    bold,
};

constexpr auto all_font_styles = {FontStyle::regular, FontStyle::italic, FontStyle::bold};

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
    stable_center,  // centers the baseline, the same for changing text
    center,
    top,
    bottom,
};

template <>
auto format(VerticalAlignment alignment) -> std::string;

// Store an object for each font style
template <typename ValueType, typename ReferenceType>
struct FontStyleCollection {
    using value_type = ValueType;
    using reference_type = ReferenceType;
    using const_reference_type = add_const_to_reference_t<ReferenceType>;

    value_type regular {};
    value_type italic {};
    value_type bold {};

    auto get(FontStyle style) const -> const_reference_type {
        switch (style) {
            using enum FontStyle;

            case regular:
                return this->regular;
            case italic:
                return this->italic;
            case bold:
                return this->bold;
        }
        throw_exception("unknown FontStyle");
    }

   protected:
    auto get(FontStyle style) -> reference_type {
        switch (style) {
            using enum FontStyle;

            case regular:
                return this->regular;
            case italic:
                return this->italic;
            case bold:
                return this->bold;
        }
        throw_exception("unknown FontStyle");
    }

   protected:
    auto set(FontStyle style, value_type value) -> void {
        switch (style) {
            using enum FontStyle;

            case regular:
                this->regular = std::move(value);
                return;
            case italic:
                this->italic = std::move(value);
                return;
            case bold:
                this->bold = std::move(value);
                return;
        }
        throw_exception("unknown FontStyle");
    }
};

}  // namespace logicsim

#endif