#include "validate_definition_decoration.h"

#include "layout_info.h"
#include "vocabulary/decoration_definition.h"

namespace logicsim {

auto is_valid(const attributes_text_element_t& a) -> bool {
    static_cast<void>(a);
    return true;
}

auto is_valid(const DecorationDefinition& d) -> bool {
    using enum DecorationType;

    if (!is_decoration_size_valid(d.decoration_type, d.width, d.height)) {
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
