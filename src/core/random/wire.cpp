#include "core/random/wire.h"

#include "core/algorithm/range.h"
#include "core/algorithm/uniform_int_distribution.h"
#include "core/editable_circuit.h"
#include "core/geometry/part.h"
#include "core/logging.h"
#include "core/random/bool.h"
#include "core/random/insertion_mode.h"
#include "core/random/ordered_line.h"
#include "core/random/point.h"
#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/decoration_definition.h"
#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/logicitem_type.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/orientation.h"
#include "core/vocabulary/segment_part.h"

#include <exception>

namespace logicsim {

auto add_random_wire(Rng& rng, EditableCircuit& editable_circuit, grid_t min, grid_t max,
                     grid_t max_length, bool random_modes) -> void {
    const auto line = get_random_ordered_line(rng, min, max, max_length);
    const auto mode =
        random_modes ? get_random_insertion_mode(rng) : InsertionMode::insert_or_discard;

    const auto segment_part = editable_circuit.add_wire_segment(line, mode);

    if (bool {segment_part} && distance(segment_part.part) != distance(to_part(line))) {
        throw std::runtime_error("parts have different sizes");
    }
}

auto add_random_button(Rng& rng, EditableCircuit& editable_circuit, grid_t min,
                       grid_t max, bool random_modes) -> void {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::button,
        .input_count = connection_count_t {0},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::undirected,
    };
    const auto position = get_random_point(rng, min, max);
    const auto mode =
        random_modes ? get_random_insertion_mode(rng) : InsertionMode::insert_or_discard;

    editable_circuit.add_logicitem(definition, position, mode);
}

auto add_random_text(Rng& rng, EditableCircuit& editable_circuit, grid_t min, grid_t max,
                     bool random_modes) -> void {
    const auto definition = DecorationDefinition {
        .decoration_type = DecorationType::text_element,
        .size = size_2d_t {0, 0},
        .attrs_text_element = attributes_text_element_t {.text = ""},
    };
    const auto position = get_random_point(rng, min, max);
    const auto mode =
        random_modes ? get_random_insertion_mode(rng) : InsertionMode::insert_or_discard;

    editable_circuit.add_decoration(definition, position, mode);
}

auto add_many_wires(Rng& rng, EditableCircuit& editable_circuit, bool random_modes,
                    int max_tries) -> void {
    const auto min = grid_t {5};
    const auto max = grid_t {10};
    const auto length = max - min;

    const auto tries = std::min(max_tries, uint_distribution(5, 100)(rng));

    for (auto _ [[maybe_unused]] : range(tries)) {
        add_random_wire(rng, editable_circuit, min, max, length, random_modes);
    }
}

auto add_many_wires_and_buttons(Rng& rng, EditableCircuit& editable_circuit,
                                WiresButtonsParams params) -> void {
    const auto min = params.grid_start;
    const auto max = params.grid_end;
    const auto length = params.max_length;

    const auto tries = uint_distribution(params.tries_start, params.tries_end)(rng);

    for (auto _ [[maybe_unused]] : range(tries)) {
        if (get_random_bool(rng, 0.1)) {
            if (get_random_bool(rng, 0.8)) {
                add_random_button(rng, editable_circuit, min, max, params.random_modes);
            } else {
                add_random_text(rng, editable_circuit, min, max, params.random_modes);
            }
        } else {
            add_random_wire(rng, editable_circuit, min, max, length, params.random_modes);
        }
    }
}

}  // namespace logicsim
