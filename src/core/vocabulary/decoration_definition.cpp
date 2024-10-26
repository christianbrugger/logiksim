#include "core/vocabulary/decoration_definition.h"

#include "core/allocated_size/std_string.h"

#include <fmt/core.h>

namespace logicsim {

auto attributes_text_element_t::format() const -> std::string {
    return fmt::format("(text = \"{}\", alignment = {}, style = {}, text_color = {})",
                       text, horizontal_alignment, font_style, text_color);
}

auto attributes_text_element_t::allocated_size() const -> std::size_t {
    return get_allocated_size(text);
}

auto DecorationDefinition::format() const -> std::string {
    const auto attr_str =
        attrs_text_element ? fmt::format(", attrs_text_element={}", *attrs_text_element)
                           : std::string {};

    return fmt::format("LogicItemDefinition({} {} {})", decoration_type, size, attr_str);
}

}  // namespace logicsim
