#include "layout_info.h"

#include "geometry/grid.h"
#include "geometry/layout_calculation.h"
#include "geometry/orientation.h"
#include "geometry/rect.h"
#include "logic_item/layout.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"
#include "vocabulary/rect.h"
#include "vocabulary/rect_fine.h"

namespace logicsim {

//
// Constants
//

namespace defaults {
constexpr static inline auto line_selection_padding = grid_fine_t {0.3};
constexpr static inline auto logic_item_body_overdraw = grid_fine_t {0.4};
constexpr static inline auto button_body_overdraw = grid_fine_t {0.5};
}  // namespace defaults

auto line_selection_padding() -> grid_fine_t {
    return defaults::line_selection_padding;
}

auto logic_item_body_overdraw() -> grid_fine_t {
    return defaults::logic_item_body_overdraw;
}

auto button_body_overdraw() -> grid_fine_t {
    return defaults::button_body_overdraw;
}

//
// Validation
//

auto is_input_output_count_valid(ElementType element_type, connection_count_t input_count,
                                 connection_count_t output_count) -> bool {
    const auto info = get_layout_info(element_type);

    return info.input_count_min <= input_count && input_count <= info.input_count_max &&
           info.output_count_min <= output_count && output_count <= info.output_count_max;
}

auto is_orientation_valid(ElementType element_type, orientation_t orientation) -> bool {
    const auto info = get_layout_info(element_type);

    switch (info.direction_type) {
        using enum DirectionType;
        case undirected:
            return orientation == orientation_t::undirected;
        case directed:
            return orientation != orientation_t::undirected;
        case any:
            return true;
    }
    std::terminate();
}

auto is_representable(layout_calculation_data_t data) -> bool {
    if (!is_logic_item(data.element_type)) {
        throw std::runtime_error("Only supported for logic items.");
    }

    const auto position = data.position;
    data.position = point_t {0, 0};
    const auto rect = element_collision_rect(data);

    static_assert(sizeof(int) > sizeof(grid_t::value_type));
    return is_representable(int {position.x} + int {rect.p0.x},
                            int {position.y} + int {rect.p0.y}) &&
           is_representable(int {position.x} + int {rect.p1.x},
                            int {position.y} + int {rect.p1.y});
}

auto is_valid(const layout_calculation_data_t &data) -> bool {
    return is_input_output_count_valid(data.element_type, data.input_count,
                                       data.output_count) &&
           is_orientation_valid(data.element_type, data.orientation) &&
           is_representable(data);
}

//
//
//

auto element_input_count_min(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).input_count_min;
}

auto element_input_count_max(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).input_count_max;
}

auto element_input_count_default(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).input_count_default;
}

auto element_output_count_min(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).output_count_min;
}

auto element_output_count_max(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).output_count_max;
}

auto element_output_count_default(ElementType element_type) -> connection_count_t {
    return get_layout_info(element_type).output_count_default;
}

auto element_direction_type(ElementType element_type) -> DirectionType {
    return get_layout_info(element_type).direction_type;
}

auto element_fixed_width(ElementType element_type) -> grid_t {
    const auto info = get_layout_info(element_type);
    if (info.variable_width) [[unlikely]] {
        throw std::runtime_error("element has variable width");
    }
    return info.fixed_width;
}

auto element_fixed_height(ElementType element_type) -> grid_t {
    const auto info = get_layout_info(element_type);
    if (info.variable_height) [[unlikely]] {
        throw std::runtime_error("element has variable height");
    }
    return info.fixed_height;
}

auto element_fixed_size(ElementType element_type) -> point_t {
    return point_t {element_fixed_width(element_type),
                    element_fixed_height(element_type)};
}

auto element_width(const layout_calculation_data_t &data) -> grid_t {
    if (!is_logic_item(data.element_type)) {
        throw std::runtime_error("Only supported for logic items");
    }
    const auto info = get_layout_info(data.element_type);
    return info.variable_width ? info.variable_width(data) : info.fixed_width;
}

auto element_height(const layout_calculation_data_t &data) -> grid_t {
    if (!is_logic_item(data.element_type)) {
        throw std::runtime_error("Only supported for logic items");
    }
    const auto info = get_layout_info(data.element_type);
    return info.variable_height ? info.variable_height(data) : info.fixed_height;
}

auto element_size(const layout_calculation_data_t &data) -> point_t {
    return point_t {element_width(data), element_height(data)};
}

auto element_body_draw_rect_untransformed(const layout_calculation_data_t &data)
    -> rect_fine_t {
    const auto size = element_size(data);

    if (data.element_type == ElementType::button) {
        const auto padding = defaults::button_body_overdraw;
        return rect_fine_t {
            point_fine_t {-padding, -padding},
            point_fine_t {size.x + padding, size.y + padding},
        };
    }

    return rect_fine_t {
        point_fine_t {0., -defaults::logic_item_body_overdraw},
        point_fine_t {size.x, size.y + defaults::logic_item_body_overdraw},
    };
}

auto element_body_draw_rect(const layout_calculation_data_t &data) -> rect_fine_t {
    const auto rect = element_body_draw_rect_untransformed(data);
    return transform(data.position, data.orientation, rect);
}

auto element_collision_rect(const layout_calculation_data_t &data) -> rect_t {
    if (!is_logic_item(data.element_type)) {
        throw std::runtime_error("Only supported for logic items");
    }
    const auto rect = rect_t {point_t {0, 0}, element_size(data)};

    return transform(data.position, data.orientation, rect);
}

auto element_bounding_rect(const layout_calculation_data_t &data) -> rect_t {
    if (!is_logic_item(data.element_type)) {
        throw std::runtime_error("Only supported for logic items");
    }
    return enclosing_rect(element_selection_rect(data));
}

auto element_selection_rect(const layout_calculation_data_t &data) -> rect_fine_t {
    constexpr static auto overdraw = grid_fine_t {0.5};

    const auto rect = element_collision_rect(data);

    return rect_fine_t {
        point_fine_t {rect.p0.x - overdraw, rect.p0.y - overdraw},
        point_fine_t {rect.p1.x + overdraw, rect.p1.y + overdraw},
    };
}

auto element_selection_rect(ordered_line_t line) -> rect_fine_t {
    constexpr auto padding = grid_fine_t {defaults::line_selection_padding};

    const auto p0 = point_fine_t {line.p0};
    const auto p1 = point_fine_t {line.p1};

    if (is_horizontal(line)) {
        return rect_fine_t {
            point_fine_t {p0.x, p0.y - padding},
            point_fine_t {p1.x, p1.y + padding},
        };
    }
    if (is_vertical(line)) {
        return rect_fine_t {
            point_fine_t {p0.x - padding, p0.y},
            point_fine_t {p1.x + padding, p1.y},
        };
    }
    return rect_fine_t {p0, p1};
}

auto element_shadow_rect(const layout_calculation_data_t &data) -> rect_fine_t {
    if (!is_logic_item(data.element_type)) {
        throw std::runtime_error("Only supported for logic items");
    }
    return element_selection_rect(data);
}

auto element_shadow_rect(ordered_line_t line) -> rect_fine_t {
    constexpr auto padding = grid_fine_t {defaults::line_selection_padding};

    const auto p0 = point_fine_t {line.p0};
    const auto p1 = point_fine_t {line.p1};

    return rect_fine_t {
        point_fine_t {p0.x - padding, p0.y - padding},
        point_fine_t {p1.x + padding, p1.y + padding},
    };
}

}  // namespace logicsim
