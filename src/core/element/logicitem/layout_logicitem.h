#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_H

#include "core/container/static_vector.h"
#include "core/element/logicitem/layout_logicitem_display_ascii.h"
#include "core/element/logicitem/layout_logicitem_display_number.h"
#include "core/element/logicitem/layout_logicitem_standard_element.h"
#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/connector_info.h"
#include "core/vocabulary/direction_type.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/layout_info_vector.h"
#include "core/vocabulary/logicitem_type.h"
#include "core/vocabulary/point.h"

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

    std::optional<grid_t> fixed_width {};
    std::optional<grid_t> fixed_height {};
    grid_t (*variable_width)(const layout_calculation_data_t&) {nullptr};
    grid_t (*variable_height)(const layout_calculation_data_t&) {nullptr};

    /**
     * @brief: static inputs and outputs with positions and orientation.
     *
     * Note simulation only connectors without positions are ommitted.
     */
    std::optional<static_inputs_t> static_inputs {};
    std::optional<static_outputs_t> static_outputs {};

    /**
     * @brief: enable inputs are turned on automatically during simulation
     */
    std::optional<connection_id_t> enable_input_id {};
};

/**
 * @brief: Get the basic layout information about the type.
 *
 * Note that if an element has dynamic inputs / outputs those cannot be
 * defined here and need to befined in the base methods below.
 *
 * Everything else should be defined here.
 */
constexpr inline auto get_layout_info(LogicItemType logicitem_type) -> layout_info_t {
    switch (logicitem_type) {
        using enum LogicItemType;

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

                .static_inputs =
                    static_inputs_t {
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                    },
                .static_outputs =
                    static_outputs_t {
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

                .static_inputs = static_inputs_t {},
                .static_outputs =
                    static_outputs_t {
                        {.position = point_t {0, 0},
                         .orientation = orientation_t::undirected},
                    },
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

                .static_inputs =
                    static_inputs_t {
                        {.position = point_t {0, 0},
                         .orientation = orientation_t::undirected},
                    },
                .static_outputs = static_outputs_t {},

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

                .enable_input_id = connection_id_t {0},
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

                .static_inputs = display_ascii::static_inputs,
                .static_outputs = static_outputs_t {},
                .enable_input_id = display::enable_input_id,
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
                .static_inputs =
                    static_inputs_t {
                        {.position = point_t {3, 4}, .orientation = orientation_t::down},
                    },
                .static_outputs =
                    static_outputs_t {
                        {.position = point_t {5, 2}, .orientation = orientation_t::right},
                    },
                .enable_input_id = display::enable_input_id,
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

                .static_inputs =
                    static_inputs_t {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // j & k
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        {.position = point_t {0, 2}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .static_outputs =
                    static_outputs_t {
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

                .static_inputs =
                    static_inputs_t {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // inputs
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        {.position = point_t {0, 2}, .orientation = orientation_t::left},
                    },
                .static_outputs =
                    static_outputs_t {
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

                .static_inputs =
                    static_inputs_t {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                    },
                .static_outputs =
                    static_outputs_t {
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

                .static_inputs =
                    static_inputs_t {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .static_outputs =
                    static_outputs_t {
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

                .static_inputs =
                    static_inputs_t {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .static_outputs =
                    static_outputs_t {
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
using static_body_points_t = static_vector<point_t, static_body_point_count, uint32_t>;

/**
 * @brief: Return the static body points.
 *
 * Note that elements with dynamic width or height don't have static body points.
 *
 * Returns empty array for element types with dynamic body points.
 */
[[nodiscard]] auto static_body_points_base(LogicItemType logicitem_type)
    -> const std::optional<static_body_points_t>&;

/**
 * @brief: Returns vector of simple_input_info_t.
 *
 * Note this is the base version, not considering element position or orientation.
 */
[[nodiscard]] auto input_locations_base(const layout_calculation_data_t& data)
    -> inputs_vector;

/**
 * @brief: Returns vector of simple_output_info_t.
 *
 * Note this is the base version, not considering element position or orientation.
 */
[[nodiscard]] auto output_locations_base(const layout_calculation_data_t& data)
    -> outputs_vector;

/**
 * @brief: Returns vector of body points, type point_t.
 *
 * Note this is the base version, not considering element position or orientation.
 */
[[nodiscard]] auto element_body_points_base(const layout_calculation_data_t& data)
    -> body_points_vector;

namespace layout_info {
[[nodiscard]] auto is_input_output_count_valid(LogicItemType logicitem_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;
}

}  // namespace logicsim

#endif
