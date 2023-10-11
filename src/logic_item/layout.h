#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_H

#include "geometry/grid.h"
#include "logic_item/layout_display_ascii.h"
#include "logic_item/layout_display_number.h"
#include "logic_item/layout_standard_element.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/connector_info.h"
#include "vocabulary/connector_static.h"
#include "vocabulary/element_type.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/point.h"

#include <exception>

namespace logicsim {

enum class DirectionType {
    undirected,
    directed,
    any,
};

// TODO format

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

    static_connectors input_connectors_fixed {};
    static_connectors output_connectors_fixed {};
};

constexpr auto get_layout_info(ElementType element_type) -> layout_info_t {
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

                .input_connectors_fixed = {{.position = point_t {0, 0},
                                            .orientation = orientation_t::left}},
                .output_connectors_fixed = {{.position = point_t {1, 0},
                                             .orientation = orientation_t::right}},
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

                .input_connectors_fixed = {},
                .output_connectors_fixed = {{.position = point_t {0, 0},
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

                .input_connectors_fixed = {{.position = point_t {0, 0},
                                            .orientation = orientation_t::undirected}},
                .output_connectors_fixed = {},

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

                .input_connectors_fixed = display_ascii::input_connectors,
                .output_connectors_fixed = {},
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
                .input_connectors_fixed = {{.position = point_t {3, 4},
                                            .orientation = orientation_t::down}},
                .output_connectors_fixed = {{.position = point_t {5, 2},
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

                .input_connectors_fixed =
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
                .output_connectors_fixed =
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

                .input_connectors_fixed =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // inputs
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        {.position = point_t {0, 2}, .orientation = orientation_t::left},
                    },
                .output_connectors_fixed =
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

                .input_connectors_fixed =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                    },
                .output_connectors_fixed =
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

                .input_connectors_fixed =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .output_connectors_fixed =
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

                .input_connectors_fixed =
                    {
                        // clock
                        {.position = point_t {0, 1}, .orientation = orientation_t::left},
                        // data
                        {.position = point_t {0, 0}, .orientation = orientation_t::left},
                        // set & reset
                        {.position = point_t {2, 0}, .orientation = orientation_t::up},
                        {.position = point_t {2, 2}, .orientation = orientation_t::down},
                    },
                .output_connectors_fixed =
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

}  // namespace logicsim

#endif
