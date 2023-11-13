#include "random/wire.h"

#include "algorithm/range.h"
#include "algorithm/uniform_int_distribution.h"
#include "component/editable_circuit/handler.h"
#include "geometry/part.h"
#include "random/bool.h"
#include "random/insertion_mode.h"
#include "random/ordered_line.h"
#include "random/point.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/logicitem_definition.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/logicitem_type.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/orientation.h"
#include "vocabulary/segment_part.h"

#include <exception>

namespace logicsim::editable_circuit {

auto add_random_wire(Rng& rng, State state, grid_t min, grid_t max, grid_t max_length,
                     bool random_modes) -> void {
    const auto line = get_random_ordered_line(rng, min, max, max_length);
    const auto mode =
        random_modes ? get_random_insertion_mode(rng) : InsertionMode::insert_or_discard;

    const auto segment_part = add_wire_segment(state, line, mode);

    if (bool {segment_part} && distance(segment_part.part) != distance(to_part(line))) {
        throw std::runtime_error("parts have different sizes");
    }
}

auto add_random_button(Rng& rng, State state, grid_t min, grid_t max, bool random_modes)
    -> void {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::button,
        .input_count = connection_count_t {0},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::undirected,
    };
    const auto position = get_random_point(rng, min, max);
    const auto mode =
        random_modes ? get_random_insertion_mode(rng) : InsertionMode::insert_or_discard;

    add_logic_item(state, definition, position, mode);
}

auto add_many_wires(Rng& rng, State state, bool random_modes, int max_tries) -> void {
    const auto min = grid_t {5};
    const auto max = grid_t {10};
    const auto length = max - min;

    const auto tries = std::min(max_tries, uint_distribution(5, 100)(rng));

    for (auto _ [[maybe_unused]] : range(tries)) {
        add_random_wire(rng, state, min, max, length, random_modes);
    }
}

auto add_many_wires_and_buttons(Rng& rng, State state, WiresButtonsParams params)
    -> void {
    const auto min = params.grid_start;
    const auto max = params.grid_end;
    const auto length = params.max_length;

    const auto tries = uint_distribution(params.tries_start, params.tries_end)(rng);

    for (auto _ [[maybe_unused]] : range(tries)) {
        if (get_random_bool(rng, 0.1)) {
            add_random_button(rng, state, min, max, params.random_modes);
        } else {
            add_random_wire(rng, state, min, max, length, params.random_modes);
        }
    }
}

}  // namespace logicsim::editable_circuit
