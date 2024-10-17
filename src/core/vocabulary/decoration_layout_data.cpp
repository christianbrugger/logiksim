#include "core/vocabulary/decoration_layout_data.h"

#include "core/vocabulary/decoration_definition.h"

namespace logicsim {

auto decoration_layout_data_t::format() const -> std::string {
    return fmt::format(
        "decoration_layout_data_t("
        "position = {}, size = {}, type={}"
        ")",
        position, size, decoration_type);
}

//
// Conversion
//

auto to_decoration_layout_data(const DecorationDefinition& definition,
                               point_t position) -> decoration_layout_data_t {
    return decoration_layout_data_t {
        .position = position,
        .size = definition.size,
        .decoration_type = definition.decoration_type,
    };
}

}  // namespace logicsim
