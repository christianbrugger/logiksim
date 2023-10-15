#include "logic_item/layout.h"

#include "algorithm/contains.h"
#include "algorithm/to_underlying.h"

#include <gsl/gsl>

#include <utility>

namespace logicsim {

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

/**
 * @brief make sure we have sensible capacities for our static and small vectors
 */
static_assert(static_body_points::capacity() == max_static_body_point_count());
static_assert(body_points_vector_size >= max_static_body_point_count());
static_assert(inputs_vector_size >= static_inputs::capacity());
static_assert(outputs_vector_size >= static_outputs::capacity());

constexpr auto calculate_static_body_points(ElementType element_type)
    -> static_body_points {
    const auto info = get_layout_info(element_type);

    if (info.variable_width != nullptr || info.variable_height != nullptr) {
        return {};
    }

    const auto to_position = [](const auto& info) { return info.position; };

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
    constexpr auto size = all_element_types.size();
    auto result = std::array<static_body_points, size> {};

    for (const auto element_type : all_element_types) {
        result[to_underlying(element_type)] = calculate_static_body_points(element_type);
    }

    return result;
}

constexpr static inline auto all_static_body_points = calculate_all_static_body_points();

}  // namespace

auto static_body_points_base(ElementType element_type) -> const static_body_points& {
    return all_static_body_points[to_underlying(element_type)];
}

auto input_locations_base(const layout_calculation_data_t& data) -> inputs_vector {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element:
            return ::logicsim::standard_element::input_locations_base(data);

        case display_number:
            return ::logicsim::display_number::input_locations_base(data);

        default: {
            const auto connectors = get_layout_info(data.element_type).input_connectors;
            return inputs_vector(connectors.begin(), connectors.end());
        }
    }
    std::terminate();
}

auto output_locations_base(const layout_calculation_data_t& data) -> outputs_vector {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element:
            return ::logicsim::standard_element::output_locations_base(data);

        case display_number:
            return ::logicsim::display_number::output_locations_base(data);

        default: {
            const auto connectors = get_layout_info(data.element_type).output_connectors;
            return outputs_vector(connectors.begin(), connectors.end());
        }
    }
    std::terminate();
}

auto element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element: {
            return standard_element::element_body_points_base(data);
        }

        case display_number: {
            return display_number::element_body_points_base(data);
        }

        default: {
            const auto& points = static_body_points_base(data.element_type);
            return body_points_vector(points.begin(), points.end());
        }
    }
    std::terminate();
}

}  // namespace logicsim
