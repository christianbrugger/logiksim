#include "logic_item/layout.h"

#include "algorithm/contains.h"
#include "algorithm/to_underlying.h"
#include "container/static_vector.h"

#include <gsl/gsl>

#include <utility>

namespace logicsim {

using static_body_points = static_vector<point_t, 28, uint32_t>;

namespace {

constexpr auto count_static_body_points(ElementType element_type) -> int {
    const auto info = get_layout_info(element_type);

    if (info.variable_width != nullptr || info.variable_height != nullptr) {
        return 0;
    }

    const auto width = info.fixed_width;
    const auto height = info.fixed_height;

    const auto input_size = gsl::narrow<int>(info.input_connectors.size());
    const auto output_size = gsl::narrow<int>(info.output_connectors.size());

    return (int {width} + 1) * (int {height} + 1) - input_size - output_size;
}

constexpr auto max_static_body_point_count() -> int {
    auto result = 0;

    for (auto type : all_element_types) {
        result = std::max(result, count_static_body_points(type));
    }

    return result;
}

static_assert(static_body_points::capacity() == max_static_body_point_count());

constexpr auto calculate_static_body_points(ElementType element_type)
    -> static_body_points {
    const auto info = get_layout_info(element_type);

    if (info.variable_width != nullptr || info.variable_height != nullptr) {
        return {};
    }

    const auto to_position = [](const connector_info_t& info) { return info.position; };

    auto result = static_body_points {};

    for (const auto x : range(info.fixed_width + grid_t {1})) {
        for (const auto y : range(info.fixed_height + grid_t {1})) {
            const auto point = point_t {x, y};

            if (contains(info.input_connectors, point, to_position)) {
                continue;
            }

            if (contains(info.output_connectors, point, to_position)) {
                continue;
            }

            result.push_back(point);
        }
    }

    return result;
}

constexpr auto calculate_all_static_body_points() {
    // constexpr auto max_id =
    //     std::ranges::max(all_element_types, {}, to_underlying<ElementType>);
    //  constexpr auto size = to_underlying(max_id) + 1;

    constexpr auto size = all_element_types.size();
    auto result = std::array<static_body_points, size> {};

    for (const auto element_type : all_element_types) {
        result[to_underlying(element_type)] = calculate_static_body_points(element_type);
    }

    return result;
}

constexpr static inline auto all_static_body_points = calculate_all_static_body_points();

}  // namespace

auto get_static_body_points(ElementType element_type) -> const static_body_points& {
    return all_static_body_points[to_underlying(element_type)];
}

//
// Iterator
//

auto iter_element_body_points_base_smallvector_private(
    const layout_calculation_data_t& data) -> body_points_vector {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element: {
            return standard_element::iter_element_body_points_smallvector(data);
        }

        case display_number: {
            return display_number::iter_element_body_points_smallvector(data);
        }

        default: {
            const auto& points = get_static_body_points(data.element_type);
            return body_points_vector(points.begin(), points.end());
        }
    }
    std::terminate();
}

}  // namespace logicsim
