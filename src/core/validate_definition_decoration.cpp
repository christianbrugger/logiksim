#include "validate_definition_decoration.h"

#include "vocabulary/decoration_definition.h"

namespace logicsim {

auto is_valid(const attributes_text_element_t& a) -> bool {
    static_cast<void>(a);
    return true;
}

auto is_valid(const DecorationDefinition& d) -> bool {
    using enum DecorationType;

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
