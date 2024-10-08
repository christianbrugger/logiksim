#include "vocabulary/decoration_layout_data.h"

#include "vocabulary/decoration_definition.h"

namespace logicsim {

auto decoration_layout_data_t::format() const -> std::string {
    return fmt::format(
        "decoration_layout_data_t("
        "position = {}, width = {}, height = {}, type={}"
        ")",
        position, width, height, decoration_type);
}

//
// Conversion
//

auto to_decoration_layout_data(const DecorationDefinition& definition, point_t position)
    -> decoration_layout_data_t {
    return decoration_layout_data_t {
        .position = position,
        .width = definition.width,
        .height = definition.height,
        .decoration_type = definition.decoration_type,
    };
}

}  // namespace logicsim
