#include "element/logicitem/layout_display_number.h"

#include "algorithm/range.h"
#include "element/logicitem/layout_display.h"
#include "geometry/connection_count.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/point.h"

#include <gcem.hpp>

#include <stdexcept>

namespace logicsim {

namespace display_number {

namespace {

constexpr auto value_inputs_(connection_count_t input_count) -> connection_count_t {
    return input_count - control_inputs;
}

// WARNING: changing this function will make saves incompatible
constexpr auto _width(connection_count_t input_count) -> grid_t {
    using gcem::ceil;
    using gcem::floor;
    using gcem::log10;
    using gcem::max;

    // font dependent, gathered by running print_character_metrics()
    constexpr auto digit_size = 0.6;
    constexpr auto sign_width = 0.6;
    constexpr auto separator_width = 0.6;
    static_assert(display::font_style == FontStyle::monospace);

    // independent
    constexpr auto font_size = double {display::font_size};
    constexpr auto padding = double {display::padding_horizontal};
    constexpr auto margin = double {display::margin_horizontal};
    // lock in values we depend on
    static_assert(font_size == 0.9);
    static_assert(padding == 0.25);
    static_assert(margin == 0.2);

    const auto digit_count_2 = gsl::narrow<double>(value_inputs_(input_count).count());
    const auto digit_count_10 = ceil(max(1., digit_count_2) * log10(2));
    const auto digit_count_10_neg = ceil(max(1., digit_count_2 - 1.) * log10(2));

    // without sign
    const auto digit_width = [&](double digit_count_10_) {
        const auto separator_count_ = floor((digit_count_10_ - 1.) / 3.);
        return digit_count_10_ * digit_size + separator_count_ * separator_width;
    };

    const auto sign_effective_width = max(
        0., digit_width(digit_count_10_neg) + sign_width - digit_width(digit_count_10));

    const auto digit_width_grid =
        ceil((digit_width(digit_count_10) + sign_effective_width) * font_size +
             2 * padding + 2 * margin);

    return grid_t {gsl::narrow<grid_t::value_type>(max(3., 1. + digit_width_grid))};
}

constexpr auto _generate_widths() {
    constexpr auto count = std::size_t {max_inputs - min_inputs + connection_count_t {1}};
    auto result = std::array<grid_t::value_type, count> {};
    for (connection_count_t i = min_inputs; i <= max_inputs; ++i) {
        result.at(std::size_t {i - min_inputs}) = _width(i).value;
    }
    return result;
}

constexpr inline auto generated_widths = _generate_widths();

// lock in generated values to make sure our saves are compatible
static_assert(generated_widths ==
              std::array<grid_t::value_type, 64> {
                  3,  3,  3,  3,  4,  4,  4,  5,  5,  5,  6,  6,  6,  6,  6,  6,   //
                  6,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  10, 10,  //
                  10, 10, 10, 10, 10, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13,  //
                  13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16});

}  // namespace

auto value_inputs(connection_count_t input_count) -> connection_count_t {
    return value_inputs_(input_count);
}

auto width(connection_count_t input_count) -> grid_t {
    return grid_t {generated_widths.at((input_count - min_inputs).count())};
}

auto height(connection_count_t input_count) -> grid_t {
    return to_grid(
        std::max(connection_count_t {2}, input_count - connection_count_t {3}));
}

auto input_shift(connection_count_t input_count) -> grid_t {
    const auto space =
        width(input_count) - grid_t {1} - to_grid(display_number::control_inputs);

    static_assert(sizeof(int) > sizeof(grid_t::value_type));
    return grid_t {(int {space} + 1) / 2};
}

auto enable_position(connection_count_t input_count) -> point_t {
    return point_t {
        grid_t {2} + input_shift(input_count),
        height(input_count),
    };
}

auto negative_position(connection_count_t input_count) -> point_t {
    return point_t {
        grid_t {1} + input_shift(input_count),
        height(input_count),
    };
}

//
// Iterator
//

auto input_locations_base(const layout_calculation_data_t& data) -> inputs_vector {
    auto connectors = inputs_vector {};
    connectors.reserve(data.input_count.count());

    // enable
    static_assert(display::enable_input_id == connection_id_t {0});
    connectors.push_back({enable_position(data.input_count), orientation_t::down});

    // negative
    connectors.push_back({negative_position(data.input_count), orientation_t::down});

    // number inputs
    for (auto y : range(to_grid(value_inputs(data.input_count)))) {
        connectors.push_back({point_t {0, y}, orientation_t::left});
    }

    return connectors;
}

auto output_locations_base(const layout_calculation_data_t& data
                           [[maybe_unused]]) -> outputs_vector {
    return outputs_vector {};
}

auto element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector {
    const auto w = width(data.input_count);
    const auto h = height(data.input_count);

    const auto negative_pos = negative_position(data.input_count);
    const auto enable_pos = enable_position(data.input_count);
    const auto max_input_y = to_grid(value_inputs(data.input_count)) - grid_t {1};

    auto result = body_points_vector {};
    // TODO reserve

    for (const auto y : range(h + grid_t {1})) {
        for (const auto x : range(w + grid_t {1})) {
            const auto point = point_t {x, y};

            if (point.x == grid_t {0} && point.y <= max_input_y) {
                continue;
            }
            if (point == negative_pos || point == enable_pos) {
                continue;
            }

            result.push_back(point);
        }
    }

    return result;
}

}  // namespace display_number

}  // namespace logicsim
