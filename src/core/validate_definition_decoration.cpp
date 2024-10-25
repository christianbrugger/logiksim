#include "core/validate_definition_decoration.h"

#include "core/layout_info.h"
#include "core/vocabulary/decoration_definition.h"

namespace logicsim {

auto is_valid(const attributes_text_element_t& a) -> bool {
    return is_rgb(a.text_color);
}

auto is_valid(const DecorationDefinition& d) -> bool {
    using enum DecorationType;

    if (!is_decoration_size_valid(d.decoration_type, d.size)) {
        return false;
    }

    // Text element
    if ((d.decoration_type == text_element) != d.attrs_text_element.has_value()) {
        return false;
    }
    if (d.attrs_text_element.has_value() && !is_valid(*d.attrs_text_element)) {
        return false;
    }

    return true;
}

}  // namespace logicsim
