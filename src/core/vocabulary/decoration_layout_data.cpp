#include "vocabulary/decoration_layout_data.h"

#include "vocabulary/decoration_definition.h"

namespace logicsim {

auto decoration_layout_data_t::format() const -> std::string {
    return fmt::format(
        "decoration_layout_data_t("
        "type={}, bounding_rect={}"
        ")",
        decoration_type, bounding_rect);
}

//
// Conversion
//

auto to_decoration_layout_data(const DecorationDefinition& definition,
                               point_t position) -> decoration_layout_data_t {
    return decoration_layout_data_t {
        .bounding_rect = rect_t {position, position},  // TODO based on definition
        .decoration_type = definition.decoration_type,
    };
}

}  // namespace logicsim
