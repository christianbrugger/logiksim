#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_H

#include "geometry/grid.h"
#include "logic_item/layout_display_ascii.h"
#include "logic_item/layout_display_number.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/element_type.h"
#include "vocabulary/grid.h"
#include "vocabulary/layout_calculation_data.h"

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
    connection_count_t output_count_min {0};
    connection_count_t output_count_max {0};
    DirectionType direction_type {DirectionType::any};

    grid_t fixed_width {0};
    grid_t fixed_height {0};

    grid_t (*variable_width)(const layout_calculation_data_t&) {nullptr};
    grid_t (*variable_height)(const layout_calculation_data_t&) {nullptr};
};

constexpr auto get_layout_info(ElementType element_type) -> layout_info_t {
    switch (element_type) {
        using enum ElementType;

        case unused: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t {0},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},

                .direction_type = DirectionType::any,
            };
        }
        case placeholder: {
            return layout_info_t {
                .input_count_min = connection_count_t {1},
                .input_count_max = connection_count_t {1},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},

                .direction_type = DirectionType::any,
            };
        }
        case wire: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t {1},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t::max(),

                .direction_type = DirectionType::any,
            };
        }

        case buffer_element: {
            return layout_info_t {
                .input_count_min = connection_count_t {1},
                .input_count_max = connection_count_t {1},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {1},
                .fixed_height = grid_t {0},
            };
        }
        case and_element:
        case or_element:
        case xor_element: {
            return layout_info_t {
                .input_count_min = connection_count_t {2},
                .input_count_max = connection_count_t {128},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {2},
                .variable_height =
                    [](const layout_calculation_data_t& data) {
                        return to_grid(data.input_count - connection_count_t {1});
                    },
            };
        }

        case button: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t {0},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},

                .direction_type = DirectionType::undirected,

                .fixed_width = grid_t {0},
                .fixed_height = grid_t {0},
            };
        }
        case led: {
            return layout_info_t {
                .input_count_min = connection_count_t {1},
                .input_count_max = connection_count_t {1},

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},

                .direction_type = DirectionType::undirected,

                .fixed_width = grid_t {0},
                .fixed_height = grid_t {0},

            };
        }
        case display_number: {
            return layout_info_t {
                .input_count_min = display_number::min_inputs,
                .input_count_max = display_number::max_inputs,

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},

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

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t {0},

                .direction_type = DirectionType::directed,

                .fixed_width = display_ascii::width,
                .fixed_height = display_ascii::height,
            };
        }

        case clock_generator: {
            return layout_info_t {
                .input_count_min = connection_count_t {3},
                .input_count_max = connection_count_t {3},

                .output_count_min = connection_count_t {3},
                .output_count_max = connection_count_t {3},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {5},
                .fixed_height = grid_t {4},
            };
        }
        case flipflop_jk: {
            return layout_info_t {
                .input_count_min = connection_count_t {5},
                .input_count_max = connection_count_t {5},

                .output_count_min = connection_count_t {2},
                .output_count_max = connection_count_t {2},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {4},
                .fixed_height = grid_t {2},
            };
        }
        case shift_register: {
            return layout_info_t {
                .input_count_min = connection_count_t {3},
                .input_count_max = connection_count_t {3},

                .output_count_min = connection_count_t {2},
                .output_count_max = connection_count_t {2},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {8},
                .fixed_height = grid_t {2},
            };
        }
        case latch_d: {
            return layout_info_t {
                .input_count_min = connection_count_t {2},
                .input_count_max = connection_count_t {2},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {2},
                .fixed_height = grid_t {1},
            };
        }
        case flipflop_d: {
            return layout_info_t {
                .input_count_min = connection_count_t {4},
                .input_count_max = connection_count_t {4},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {3},
                .fixed_height = grid_t {2},
            };
        }
        case flipflop_ms_d: {
            return layout_info_t {
                .input_count_min = connection_count_t {4},
                .input_count_max = connection_count_t {4},

                .output_count_min = connection_count_t {1},
                .output_count_max = connection_count_t {1},

                .direction_type = DirectionType::directed,

                .fixed_width = grid_t {4},
                .fixed_height = grid_t {2},
            };
        }

        case sub_circuit: {
            return layout_info_t {
                .input_count_min = connection_count_t {0},
                .input_count_max = connection_count_t::max(),

                .output_count_min = connection_count_t {0},
                .output_count_max = connection_count_t::max(),

                .direction_type = DirectionType::directed,
            };
        }
    }

    std::terminate();
}

}  // namespace logicsim

#endif
