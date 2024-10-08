#include "element/decoration/layout_decoration.h"

#include "algorithm/range_extended.h"
#include "vocabulary/decoration_layout_data.h"

namespace logicsim {

namespace {

[[nodiscard]] auto decoration_body_points_text_element(
    const decoration_layout_data_t& data) -> body_points_vector {
    const auto& p0 = data.bounding_rect.p0;
    const auto& p1 = data.bounding_rect.p1;

    auto result = body_points_vector {};

    for (const auto x : range_inclusive<grid_t>(p0.x.value, p1.x.value)) {
        for (const auto y : range_inclusive<grid_t>(p0.y.value, p1.y.value)) {
            result.push_back(point_t {x, y});
        }
    }
    return result;
}

}  // namespace

auto decoration_body_points(const decoration_layout_data_t& data) -> body_points_vector {
    switch (data.decoration_type) {
        using enum DecorationType;
        case text_element:
            return decoration_body_points_text_element(data);
    }
    std::terminate();
}

}  // namespace logicsim
