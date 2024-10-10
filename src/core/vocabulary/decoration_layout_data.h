#ifndef LOGICSIM_CORE_VOCABULARY_DECORATION_LAYOUT_DATA_H
#define LOGICSIM_CORE_VOCABULARY_DECORATION_LAYOUT_DATA_H

#include "vocabulary/decoration_type.h"
#include "vocabulary/point.h"
#include "vocabulary/size_2d.h"

namespace logicsim {

struct DecorationDefinition;

/**
 * @brief: Decoration data required to calculate its layout.
 */
struct decoration_layout_data_t {
    point_t position;
    size_2d_t size;
    DecorationType decoration_type {DecorationType::text_element};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const decoration_layout_data_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const decoration_layout_data_t& other) const = default;
};

static_assert(std::is_aggregate_v<decoration_layout_data_t>);

//
// Public
//

[[nodiscard]] auto to_decoration_layout_data(
    const DecorationDefinition& definition, point_t position) -> decoration_layout_data_t;

}  // namespace logicsim

#endif
