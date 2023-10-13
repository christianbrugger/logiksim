#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_NUMBER_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_NUMBER_H

#include "algorithm/range.h"
#include "geometry/connection_count.h"
#include "iterator_adaptor/polling_iterator.h"
#include "logic_item/layout_display.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <concepts>
#include <cppcoro/generator.hpp>

namespace logicsim {

namespace display_number {

constexpr static inline auto control_inputs = connection_count_t {2};
constexpr static inline auto min_value_inputs = connection_count_t {1};
constexpr static inline auto max_value_inputs = connection_count_t {64};
constexpr static inline auto default_value_inputs = connection_count_t {3};

constexpr static inline auto min_inputs = control_inputs + min_value_inputs;
constexpr static inline auto max_inputs = control_inputs + max_value_inputs;
constexpr static inline auto default_inputs = control_inputs + default_value_inputs;

[[nodiscard]] auto value_inputs(connection_count_t input_count) -> connection_count_t;
[[nodiscard]] auto width(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto height(connection_count_t input_count) -> grid_t;

[[nodiscard]] auto input_shift(connection_count_t input_count) -> grid_t;
[[nodiscard]] auto enable_position(connection_count_t input_count) -> point_t;
[[nodiscard]] auto negative_position(connection_count_t input_count) -> point_t;
constexpr static inline auto negative_input_id = connection_id_t {1};

/**
 * @brief: Iterate over the inputs of standard elements
 *         not considering position or orientation.
 *
 *  next_input = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
inline auto iter_input_location(const layout_calculation_data_t& data,
                                std::invocable<point_t, orientation_t> auto next_input)
    -> bool;

/**
 * @brief: Iterate over the outputs of standard elements
 *         not considering position or orientation.
 *
 *  next_output = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
inline auto iter_output_location(const layout_calculation_data_t& data,
                                 std::invocable<point_t, orientation_t> auto next_output)
    -> bool;

/**
 * @brief: Iterate over the body points of standard elements
 *         not considering position or orientation.
 *
 *  next_point = [](point_t position) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
inline auto iter_element_body_points(const layout_calculation_data_t& data,
                                     std::invocable<point_t> auto next_point) -> bool;

[[nodiscard]] inline auto iter_element_body_points(const layout_calculation_data_t& data)
    -> cppcoro::generator<const point_t>;

//
// Implementation
//

// next_input = [](point_t position, orientation_t orientation) -> bool
inline auto iter_input_location(const layout_calculation_data_t& data,
                                std::invocable<point_t, orientation_t> auto next_input)
    -> bool {
    // enable
    static_assert(display::enable_input_id == connection_id_t {0});
    if (!next_input(enable_position(data.input_count), orientation_t::down)) {
        return false;
    }

    // negative
    static_assert(display_number::negative_input_id == connection_id_t {1});
    if (!next_input(negative_position(data.input_count), orientation_t::down)) {
        return false;
    }

    // number inputs
    for (auto y : range(to_grid(value_inputs(data.input_count)))) {
        if (!next_input(point_t {0, y}, orientation_t::left)) {
            return false;
        }
    }
    return true;
}

// next_output = [](point_t position, orientation_t orientation) -> bool
inline auto iter_output_location(const layout_calculation_data_t& data,
                                 std::invocable<point_t, orientation_t> auto next_output)
    -> bool {
    return true;
}

// next_point = [](point_t position) -> bool
inline auto iter_element_body_points(const layout_calculation_data_t& data,
                                     std::invocable<point_t> auto next_point) -> bool {
    const auto w = width(data.input_count);
    const auto h = height(data.input_count);

    const auto negative_pos = negative_position(data.input_count);
    const auto enable_pos = enable_position(data.input_count);
    const auto max_input_y = to_grid(value_inputs(data.input_count)) - grid_t {1};

    for (const auto y : range(h + grid_t {1})) {
        for (const auto x : range(w + grid_t {1})) {
            const auto point = point_t {x, y};

            if (point.x == grid_t {0} && point.y <= max_input_y) {
                continue;
            }
            if (point == negative_pos || point == enable_pos) {
                continue;
            }

            if (!next_point(point)) {
                return false;
            }
        }
    }
    return true;
}

inline auto iter_element_body_points(const layout_calculation_data_t& data)
    -> cppcoro::generator<const point_t> {
    const auto w = width(data.input_count);
    const auto h = height(data.input_count);

    const auto negative_pos = negative_position(data.input_count);
    const auto enable_pos = enable_position(data.input_count);
    const auto max_input_y = to_grid(value_inputs(data.input_count)) - grid_t {1};

    for (const auto y : range(h + grid_t {1})) {
        for (const auto x : range(w + grid_t {1})) {
            const auto point = point_t {x, y};

            if (point.x == grid_t {0} && point.y <= max_input_y) {
                continue;
            }
            if (point == negative_pos || point == enable_pos) {
                continue;
            }

            co_yield point;
        }
    }
}

inline auto iter_element_body_points_vector(const layout_calculation_data_t& data)
    -> std::vector<point_t> {
    auto result = std::vector<point_t> {};

    const auto w = width(data.input_count);
    const auto h = height(data.input_count);

    const auto negative_pos = negative_position(data.input_count);
    const auto enable_pos = enable_position(data.input_count);
    const auto max_input_y = to_grid(value_inputs(data.input_count)) - grid_t {1};

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

struct bp_state {
    point_t point {};
    grid_t width {};
    grid_t height {};

    point_t negative_pos {};
    point_t enable_pos {};
    grid_t max_input_y {};

    auto operator==(const bp_state&) const -> bool = default;
};

using bp_view = polling_view<point_t, bp_state>;

inline auto iter_element_body_points_polling(const layout_calculation_data_t& data)
    -> bp_view {
    const auto state = bp_state {
        .point = point_t {1, 0},
        .width = width(data.input_count),
        .height = height(data.input_count),

        .negative_pos = negative_position(data.input_count),
        .enable_pos = enable_position(data.input_count),
        .max_input_y = to_grid(value_inputs(data.input_count)) - grid_t {1},
    };

    const auto mutator = [](bp_state& state) -> polling_status {
        const auto inc = [&] {
            ++state.point.x;
            if (state.point.x > state.width) {
                state.point.x = grid_t {0};
                ++state.point.y;
            }
        };

        inc();

        while (state.point.x == grid_t {0} && state.point.y <= state.max_input_y) {
            inc();
        }
        while (state.point == state.negative_pos || state.point == state.enable_pos) {
            inc();
        }

        return state.point.y > state.height ? polling_status::stop
                                            : polling_status::iterate;
    };

    const auto getter = [](const bp_state& s) { return s.point; };

    return bp_view {mutator, getter, state, polling_status::iterate};
}

}  // namespace display_number

}  // namespace logicsim

#endif
