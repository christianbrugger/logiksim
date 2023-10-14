#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_H

#include "logic_item/layout_display_ascii.h"
#include "logic_item/layout_display_number.h"
#include "logic_item/layout_standard_element.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/connector_info.h"
#include "vocabulary/direction_type.h"
#include "vocabulary/element_type.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/layout_info_small_vector.h"
#include "vocabulary/point.h"

#include <concepts>
#include <exception>
#include <optional>

namespace logicsim {

/**
 * @brief: Local type to store all layout information of a specific element type.
 */
struct layout_info_t {
    connection_count_t input_count_min {0};
    connection_count_t input_count_max {0};
    connection_count_t input_count_default {0};

    connection_count_t output_count_min {0};
    connection_count_t output_count_max {0};
    connection_count_t output_count_default {0};

    DirectionType direction_type {DirectionType::any};

    grid_t fixed_width {0};
    grid_t fixed_height {0};

    grid_t (*variable_width)(const layout_calculation_data_t&) {nullptr};
    grid_t (*variable_height)(const layout_calculation_data_t&) {nullptr};

    static_connectors input_connectors {};
    static_connectors output_connectors {};
};

/**
 * @brief: Get the basic layout information about the type.
 *
 * Note that if an element has dynamic inputs / outputs those cannot be
 * defined here and need to befined in the base methods below.
 *
 * Everything else should be defined here.
 */
constexpr inline auto get_layout_info(ElementType element_type) -> layout_info_t {
    switch (element_type) {
        using enum ElementType;

        case unused: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t {0},
                .input_count_default = connection_count_t {0},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::any,
            };
        }
        case placeholder: {
            return layout_info_t {
                .input_count_min = connection_count_t {1},
                .input_count_max = connection_count_t {1},
                .input_count_default = connection_count_t {1},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::any,
            };
        }
        case wire: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t {1},
                .input_count_default = connection_count_t {0},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t::max(),
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::any,
            };
        }

        case buffer_element: {
            return layout_info_t {
                .input_count_min = connection_count_t {1},
                .input_count_max = connection_count_t {1},
                .input_count_default = connection_count_t {1},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},
                .output_count_default = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {1},
                .fixed_height = grid_t {0},

                .input_connectors = {{.position = point_t {0, 0},
                                      .orientation = orientation_t::left}},
                .output_connectors =
                    {
                        {.position = point_t {1, 0}, .orientation = orientation_t::right},
                    },
            };
        }
        case and_element:
        case or_element:
        case xor_element: {
            return layout_info_t {
                .input_count_min = standard_element::min_inputs,
                .input_count_max = standard_element::max_inputs,
                .input_count_default = standard_element::default_inputs,

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},
                .output_count_default = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = standard_element::width,
                .variable_height =
                    [](const layout_calculation_data_t& data) {
                        return standard_element::height(data.input_count);
                    },
            };
        }

        case button: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t {0},
                .input_count_default = connection_count_t {0},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},
                .output_count_default = connection_count_t {1},

                .direction_type = DirectionType::undirected,

                .fixed_width = grid_t {0},
                .fixed_height = grid_t {0},

                .input_connectors = {},
                .output_connectors = {{.position = point_t {0, 0},
                                       .orientation = orientation_t::undirected}},
            };
        }
        case led: {
            return layout_info_t {
                .input_count_min = connection_count_t {1},
                .input_count_max = connection_count_t {1},
                .input_count_default = connection_count_t {1},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::undirected,

                .fixed_width = grid_t {0},
                .fixed_height = grid_t {0},

                .input_connectors = {{.position = point_t {0, 0},
                                      .orientation = orientation_t::undirected}},
                .output_connectors = {},

            };
        }
        case display_number: {
            return layout_info_t {
                .input_count_min = display_number::min_inputs,
                .input_count_max = display_number::max_inputs,
                .input_count_default = display_number::default_inputs,

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::directed,

                .variable_width =
                    [](const layout_calculation_data_t& data) {
                        return display_number::width(data.input_count);
                    },
                .variable_height =
                    [](const layout_calculation_data_t& data) {
                        return display_number::height(data.input_count);
                    },
            };
        }
        case display_ascii: {
            return layout_info_t {
                .input_count_min = display_ascii::input_count,
                .input_count_max = display_ascii::input_count,
                .input_count_default = display_ascii::input_count,

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::directed,

                .fixed_width = display_ascii::width,
                .fixed_height = display_ascii::height,

                .input_connectors = display_ascii::input_connectors,
                .output_connectors = {},
            };
        }

        case clock_generator: {
            return layout_info_t {
                .input_count_min = connection_count_t {3},
                .input_count_max = connection_count_t {3},
                .input_count_default = connection_count_t {3},

                .output_count_min = connection_count_t {3},
                .output_count_max = connection_count_t {3},
                .output_count_default = connection_count_t {3},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {5},
                .fixed_height = grid_t {4},

                // the second annd third inputs and outputs are used only for simulation
                .input_connectors = {{.position = point_t {3, 4},
                                      .orientation = orientation_t::down}},
                .output_connectors = {{.position = point_t {5, 2},
                                       .orientation = orientation_t::right}},
            };
        }
        case flipflop_jk: {
            return layout_info_t {
                .input_count_min = connection_count_t {5},
                .input_count_max = connection_count_t {5},
                .input_count_default = connection_count_t {5},

                .output_count_min = connection_count_t {2},
                .output_count_max = connection_count_t {2},
                .output_count_default = connection_count_t {2},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {4},
                .fixed_height = grid_t {2},

                .input_connectors =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // j & k
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        {.position = point_t {0, 2}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .output_connectors =
                    {
                        // Q and !Q
                        {.position = point_t {4, 0}, .orientation = orientation_t::right},
                        {.position = point_t {4, 2}, .orientation = orientation_t::right},
                    },
            };
        }
        case shift_register: {
            return layout_info_t {
                .input_count_min = connection_count_t {3},
                .input_count_max = connection_count_t {3},
                .input_count_default = connection_count_t {3},

                .output_count_min = connection_count_t {2},
                .output_count_max = connection_count_t {2},
                .output_count_default = connection_count_t {2},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {8},
                .fixed_height = grid_t {2},

                .input_connectors =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // inputs
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        {.position = point_t {0, 2}, .orientation = orientation_t::left},
                    },
                .output_connectors =
                    {
                        // Q and !Q
                        {.position = point_t {8, 0}, .orientation = orientation_t::right},
                        {.position = point_t {8, 2}, .orientation = orientation_t::right},
                    },
            };
        }
        case latch_d: {
            return layout_info_t {
                .input_count_min = connection_count_t {2},
                .input_count_max = connection_count_t {2},
                .input_count_default = connection_count_t {2},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},
                .output_count_default = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {2},
                .fixed_height = grid_t {1},

                .input_connectors =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                    },
                .output_connectors =
                    {
                        // data
                        {.position = point_t {2, 0}, .orientation = orientation_t::right},
                    },
            };
        }
        case flipflop_d: {
            return layout_info_t {
                .input_count_min = connection_count_t {4},
                .input_count_max = connection_count_t {4},
                .input_count_default = connection_count_t {4},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},
                .output_count_default = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {3},
                .fixed_height = grid_t {2},

                .input_connectors =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .output_connectors =
                    {
                        // data
                        {.position = point_t {3, 0}, .orientation = orientation_t::right},
                    },
            };
        }
        case flipflop_ms_d: {
            return layout_info_t {
                .input_count_min = connection_count_t {4},
                .input_count_max = connection_count_t {4},
                .input_count_default = connection_count_t {4},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},
                .output_count_default = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {4},
                .fixed_height = grid_t {2},

                .input_connectors =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .output_connectors =
                    {
                        // data
                        {.position = point_t {4, 0}, .orientation = orientation_t::right},
                    },
            };
        }

        case sub_circuit: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t::max(),
                .input_count_default = connection_count_t {0},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t::max(),
                .output_count_default = connection_count_t {0},

                .direction_type = DirectionType::directed,
            };
        }
    }
    std::terminate();
}

/**
 * @brief: The maximum number of static body points of any logic element.
 *
 * We generate all static body points at compile time.
 * We use this array-type to store up to the largest amount we need.
 *
 * Later in the c++ file we statically check if this number matches the maximum
 * amount. If necessary adapt this number here.
 */
constexpr static inline auto static_body_point_count = 28;
using static_body_points = static_vector<point_t, static_body_point_count, uint32_t>;

/**
 * @brief: Return the static body points.
 *
 * Note that elements with dynamic width or height don't have static body points.
 *
 * Returns empty array for element types with dynamic body points.
 */
[[nodiscard]] auto get_static_body_points_base(ElementType element_type)
    -> const static_body_points&;

constexpr inline auto iter_connectors(
    const static_connectors& connectors,
    std::invocable<point_t, orientation_t> auto next_connector) -> bool {
    for (const connector_info_t& con : connectors) {
        if (!next_connector(con.position, con.orientation)) {
            return false;
        }
    }
    return true;
}

constexpr inline auto iter_body_points(const static_body_points& body_points,
                                       std::invocable<point_t> auto next_point) -> bool {
    for (const point_t& point : body_points) {
        if (!next_point(point)) {
            return false;
        }
    }
    return true;
}

//
// Iterators
//

/**
 * @brief: Iterate over the inputs not considering position or orientation.
 *
 *  next_input = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
constexpr inline auto iter_input_location_base(
    const layout_calculation_data_t& data,
    std::invocable<point_t, orientation_t> auto next_input) -> bool {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element:
            return ::logicsim::standard_element::iter_input_location(data, next_input);

        case display_number:
            return ::logicsim::display_number::iter_input_location(data, next_input);

        default: {
            const auto connectors = get_layout_info(data.element_type).input_connectors;
            return iter_connectors(connectors, next_input);
        }
    }
    std::terminate();
}

/**
 * @brief: Iterate over the outputs not considering position or orientation.
 *
 *  next_output = [](point_t position, orientation_t orientation) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
constexpr inline auto iter_output_location_base(
    const layout_calculation_data_t& data,
    std::invocable<point_t, orientation_t> auto next_output) -> bool {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element:
            return ::logicsim::standard_element::iter_output_location(data, next_output);

        case display_number:
            return ::logicsim::display_number::iter_output_location(data, next_output);

        default: {
            const auto connectors = get_layout_info(data.element_type).output_connectors;
            return iter_connectors(connectors, next_output);
        }
    }
    std::terminate();
}

/**
 * @brief: Iterate over the body points not considering position or orientation.
 *
 *  next_point = [](point_t position) -> bool
 *
 * The callable is called for each point or until it returns false.
 */
inline auto iter_element_body_points_base(const layout_calculation_data_t& data,
                                          std::invocable<point_t> auto next_point)
    -> bool {
    switch (data.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element:
            return ::logicsim::standard_element::iter_element_body_points(data,
                                                                          next_point);

        case display_number:
            return ::logicsim::display_number::iter_element_body_points(data, next_point);

        default: {
            const auto& body_points = get_static_body_points_base(data.element_type);
            return iter_body_points(body_points, next_point);
        }
    }
    std::terminate();
}

[[nodiscard]] auto iter_input_location_base(const layout_calculation_data_t& data)
    -> connectors_vector;

[[nodiscard]] auto iter_output_location_base(const layout_calculation_data_t& data)
    -> connectors_vector;

[[nodiscard]] auto iter_element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector;

}  // namespace logicsim

#endif
