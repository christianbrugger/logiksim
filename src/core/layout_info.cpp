#include "core/layout_info.h"

#include "core/element/decoration/layout_decoration.h"
#include "core/element/logicitem/layout_logicitem.h"
#include "core/geometry/grid.h"
#include "core/geometry/layout_calculation.h"
#include "core/geometry/offset.h"
#include "core/geometry/orientation.h"
#include "core/geometry/point.h"
#include "core/geometry/rect.h"
#include "core/vocabulary/decoration_layout_data.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/rect.h"
#include "core/vocabulary/rect_fine.h"

namespace logicsim {

//
// Constants
//

namespace defaults {
constexpr static inline auto line_selection_padding = grid_fine_t {0.3};
constexpr static inline auto logicitem_body_overdraw = grid_fine_t {0.4};
constexpr static inline auto button_body_overdraw = grid_fine_t {0.5};
constexpr static inline auto element_selection_overdraw = grid_fine_t {0.5};
}  // namespace defaults

auto line_selection_padding() -> grid_fine_t {
    return defaults::line_selection_padding;
}

auto logicitem_body_overdraw() -> grid_fine_t {
    return defaults::logicitem_body_overdraw;
}

auto button_body_overdraw() -> grid_fine_t {
    return defaults::button_body_overdraw;
}

//
// Validation
//

auto is_input_output_count_valid(LogicItemType logicitem_type,
                                 connection_count_t input_count,
                                 connection_count_t output_count) -> bool {
    return layout_info::is_input_output_count_valid(logicitem_type, input_count,
                                                    output_count);
}

auto is_orientation_valid(LogicItemType logicitem_type,
                          orientation_t orientation) -> bool {
    const auto info = get_layout_info(logicitem_type);

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

auto is_decoration_size_valid(DecorationType decoration_type, size_2d_t size) -> bool {
    return layout_info::is_decoration_size_valid(decoration_type, size);
}

auto is_representable(layout_calculation_data_t data) -> bool {
    const auto position = data.position;
    data.position = point_t {0, 0};
    const auto rect = element_bounding_rect(data);

    static_assert(sizeof(int) > sizeof(grid_t::value_type));
    return is_representable(int {position.x} + int {rect.p0.x},
                            int {position.y} + int {rect.p0.y}) &&
           is_representable(int {position.x} + int {rect.p1.x},
                            int {position.y} + int {rect.p1.y});
}

auto is_representable(const decoration_layout_data_t &data) -> bool {
    return is_representable(data.position, int {data.size.width}, int {data.size.height});
}

auto is_valid(const layout_calculation_data_t &data) -> bool {
    return is_input_output_count_valid(data.logicitem_type, data.input_count,
                                       data.output_count) &&
           is_orientation_valid(data.logicitem_type, data.orientation) &&
           is_representable(data);
}

auto is_valid(const decoration_layout_data_t &data) -> bool {
    return layout_info::is_decoration_size_valid(data.decoration_type, data.size) &&
           is_representable(data);
}

//
//
//

auto element_size_min(DecorationType decoration_type) -> size_2d_t {
    return layout_info::decoration_size_min(decoration_type);
}

auto element_size_max(DecorationType decoration_type) -> size_2d_t {
    return layout_info::decoration_size_max(decoration_type);
}

//
//
//

auto element_input_count_min(LogicItemType logicitem_type) -> connection_count_t {
    return get_layout_info(logicitem_type).input_count_min;
}

auto element_input_count_max(LogicItemType logicitem_type) -> connection_count_t {
    return get_layout_info(logicitem_type).input_count_max;
}

auto element_input_count_default(LogicItemType logicitem_type) -> connection_count_t {
    return get_layout_info(logicitem_type).input_count_default;
}

auto element_output_count_min(LogicItemType logicitem_type) -> connection_count_t {
    return get_layout_info(logicitem_type).output_count_min;
}

auto element_output_count_max(LogicItemType logicitem_type) -> connection_count_t {
    return get_layout_info(logicitem_type).output_count_max;
}

auto element_output_count_default(LogicItemType logicitem_type) -> connection_count_t {
    return get_layout_info(logicitem_type).output_count_default;
}

auto element_direction_type(LogicItemType logicitem_type) -> DirectionType {
    return get_layout_info(logicitem_type).direction_type;
}

auto element_enable_input_id(LogicItemType logicitem_type)
    -> std::optional<connection_id_t> {
    return get_layout_info(logicitem_type).enable_input_id;
}

auto element_fixed_width(LogicItemType logicitem_type) -> grid_t {
    const auto info = get_layout_info(logicitem_type);

    if (info.variable_width != nullptr) [[unlikely]] {
        throw std::runtime_error("element has variable width");
    }

    return info.fixed_width.value();
}

auto element_fixed_height(LogicItemType logicitem_type) -> grid_t {
    const auto info = get_layout_info(logicitem_type);

    if (info.variable_height != nullptr) [[unlikely]] {
        throw std::runtime_error("element has variable height");
    }

    return info.fixed_height.value();
}

auto element_fixed_size(LogicItemType logicitem_type) -> point_t {
    return point_t {element_fixed_width(logicitem_type),
                    element_fixed_height(logicitem_type)};
}

auto element_width(const layout_calculation_data_t &data) -> grid_t {
    const auto info = get_layout_info(data.logicitem_type);
    return info.variable_width != nullptr ? info.variable_width(data)
                                          : info.fixed_width.value();
}

auto element_height(const layout_calculation_data_t &data) -> grid_t {
    const auto info = get_layout_info(data.logicitem_type);
    return info.variable_height != nullptr ? info.variable_height(data)
                                           : info.fixed_height.value();
}

auto element_size(const layout_calculation_data_t &data) -> point_t {
    return point_t {element_width(data), element_height(data)};
}

auto element_body_draw_rect_untransformed(const layout_calculation_data_t &data)
    -> rect_fine_t {
    const auto size = element_size(data);

    if (data.logicitem_type == LogicItemType::button) {
        const auto padding = defaults::button_body_overdraw;
        return rect_fine_t {
            point_fine_t {-padding, -padding},
            point_fine_t {size.x + padding, size.y + padding},
        };
    }

    return rect_fine_t {
        point_fine_t {0., -defaults::logicitem_body_overdraw},
        point_fine_t {size.x, size.y + defaults::logicitem_body_overdraw},
    };
}

auto element_body_draw_rect(const layout_calculation_data_t &data) -> rect_fine_t {
    const auto rect = element_body_draw_rect_untransformed(data);
    return transform(data.position, data.orientation, rect);
}

auto element_bounding_rect(const layout_calculation_data_t &data) -> rect_t {
    const auto rect = rect_t {point_t {0, 0}, element_size(data)};
    return transform(data.position, data.orientation, rect);
}

auto element_bounding_rect(const decoration_layout_data_t &data) -> rect_t {
    const auto p1 = point_t {to_grid(data.size.width, data.position.x),
                             to_grid(data.size.height, data.position.y)};
    return rect_t {data.position, p1};
}

auto element_bounding_rect(ordered_line_t line) -> rect_t {
    return rect_t {line.p0, line.p1};
}

auto element_selection_rect(const layout_calculation_data_t &data) -> rect_fine_t {
    const auto rect = element_bounding_rect(data);
    return enlarge_rect(rect, defaults::element_selection_overdraw);
}

auto element_selection_rect(const decoration_layout_data_t &data) -> rect_fine_t {
    const auto rect = element_bounding_rect(data);
    return enlarge_rect(rect, defaults::element_selection_overdraw);
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
    return element_selection_rect(data);
}

auto element_shadow_rect(const decoration_layout_data_t &data) -> rect_fine_t {
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

//
// Input & Outputs & Body Points
//

auto input_locations(const layout_calculation_data_t &data) -> inputs_vector {
    auto connectors = input_locations_base(data);

    for (auto &connector : connectors) {
        connector = simple_input_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }

    return connectors;
}

auto output_locations(const layout_calculation_data_t &data) -> outputs_vector {
    auto connectors = output_locations_base(data);

    for (auto &connector : connectors) {
        connector = simple_output_info_t {
            .position = transform(data.position, data.orientation, connector.position),
            .orientation = transform(data.orientation, connector.orientation),
        };
    }

    return connectors;
}

auto element_body_points(const layout_calculation_data_t &data) -> body_points_vector {
    auto body_points = element_body_points_base(data);

    for (auto &point : body_points) {
        point = transform(data.position, data.orientation, point);
    }

    return body_points;
}

auto element_body_points(const decoration_layout_data_t &data) -> body_points_vector {
    return layout_info::decoration_body_points(data);
}

}  // namespace logicsim
