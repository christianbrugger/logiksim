#include "element/decoration/layout_decoration.h"

#include "algorithm/range_extended.h"
#include "geometry/offset.h"
#include "vocabulary/decoration_layout_data.h"

namespace logicsim {

namespace layout_info {

[[nodiscard]] auto is_decoration_size_valid(DecorationType decoration_type,
                                            offset_t width, offset_t height) -> bool {
    switch (decoration_type) {
        using enum DecorationType;

        case text_element:
            return width >= offset_t {0} && height == offset_t {0};
    }
    std::terminate();
}

namespace {

[[nodiscard]] auto decoration_body_points_text_element(
    const decoration_layout_data_t& data) -> body_points_vector {
    const auto& p0 = data.position;
    const auto p1 = point_t {
        to_grid(data.width, data.position.x),
        to_grid(data.height, data.position.y),
    };

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

}  // namespace layout_info

}  // namespace logicsim
