#include "logic_item/layout.h"

#include "algorithm/contains.h"
#include "algorithm/to_underlying.h"

#include <gsl/gsl>

#include <utility>

namespace logicsim {

namespace {

constexpr auto count_static_body_points(ElementType element_type) -> std::optional<int> {
    const auto info = get_layout_info(element_type);

    if (!info.fixed_width || !info.fixed_height) {
        return std::nullopt;
    }
    if (!info.static_inputs || !info.static_outputs) {
        return std::nullopt;
    }

    const auto width = info.fixed_width.value();
    const auto height = info.fixed_height.value();

    const auto input_size = gsl::narrow<int>(info.static_inputs.value().size());
    const auto output_size = gsl::narrow<int>(info.static_outputs.value().size());

    return (int {width} + 1) * (int {height} + 1) - input_size - output_size;
}

constexpr auto max_static_body_point_count() -> int {
    auto result = 0;

    for (auto type : all_element_types) {
        const auto count = count_static_body_points(type);
        if (count) {
            result = std::max(result, count.value());
        }
    }

    return result;
}

/**
 * @brief make sure we have sensible capacities for our static and small vectors
 */
static_assert(static_body_points_t::capacity() == max_static_body_point_count());
static_assert(body_points_vector_size >= max_static_body_point_count());
static_assert(inputs_vector_size >= static_inputs_t::capacity());
static_assert(outputs_vector_size >= static_outputs_t::capacity());

constexpr auto calculate_static_body_points(ElementType element_type)
    -> std::optional<static_body_points_t> {
    const auto info = get_layout_info(element_type);

    if (!info.fixed_width || !info.fixed_height) {
        return std::nullopt;
    }
    if (!info.static_inputs || !info.static_outputs) {
        return std::nullopt;
    }

    const auto width = info.fixed_width.value();
    const auto height = info.fixed_height.value();

    auto result = static_body_points_t {};

    for (const auto x : range(width + grid_t {1})) {
        for (const auto y : range(height + grid_t {1})) {
            const auto point = point_t {x, y};

            const auto to_position = [](const auto& info) { return info.position; };

            if (contains(info.static_inputs.value(), point, to_position)) {
                continue;
            }

            if (contains(info.static_outputs.value(), point, to_position)) {
                continue;
            }

            result.push_back(point);
        }
    }

    return result;
}

constexpr auto calculate_all_static_body_points() {
    constexpr auto size = all_element_types.size();
    auto result = std::array<std::optional<static_body_points_t>, size> {};

    for (const auto element_type : all_element_types) {
        result[to_underlying(element_type)] = calculate_static_body_points(element_type);
    }

    return result;
}

constexpr static inline auto all_static_body_points = calculate_all_static_body_points();

}  // namespace

auto static_body_points_base(ElementType element_type)
    -> const std::optional<static_body_points_t>& {
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
            const auto static_inputs = get_layout_info(data.element_type).static_inputs;
            return inputs_vector(static_inputs.value().begin(),
                                 static_inputs.value().end());
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
            const auto static_outputs = get_layout_info(data.element_type).static_outputs;
            return outputs_vector(static_outputs.value().begin(),
                                  static_outputs.value().end());
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
            return body_points_vector(points.value().begin(), points.value().end());
        }
    }
    std::terminate();
}

}  // namespace logicsim