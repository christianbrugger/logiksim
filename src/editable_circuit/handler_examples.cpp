#include "editable_circuit/handler_examples.h"

namespace logicsim::editable_circuit::examples {

auto add_many_wires(Rng& rng, State state, bool random_modes) -> void {
    grid_t::value_type min = 5;
    grid_t::value_type max = 10;

    const auto tries = 100;

    for (auto _ [[maybe_unused]] : range(tries)) {
        const auto line = get_random_line(rng, min, max);
        const auto mode = random_modes ? get_random_insertion_mode(rng)
                                       : InsertionMode::insert_or_discard;

        const auto segment_part = add_wire_segment(state, line_t {line}, mode);

        if (bool {segment_part}
            && distance(segment_part.part) != distance(to_part(line))) {
            throw std::runtime_error("parts have different sizes");
        }
    }
}

}  // namespace logicsim::editable_circuit::examples