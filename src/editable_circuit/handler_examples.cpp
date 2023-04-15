#include "editable_circuit/handler_examples.h"

namespace logicsim::editable_circuit::examples {

auto add_many_wires(Rng& rng, State state, bool random_modes) -> void {
    const auto min = grid_t::value_type {5};
    const auto max = grid_t::value_type {10};

    const auto tries = uint_distribution(5, 100)(rng);

    for (auto _ [[maybe_unused]] : range(tries)) {
        const auto line = get_random_line(rng, min, max);
        const auto mode = random_modes ? get_random_insertion_mode(rng)
                                       : InsertionMode::insert_or_discard;

        const auto segment_part = add_wire_segment(state, line, mode);

        if (bool {segment_part}
            && distance(segment_part.part) != distance(to_part(line))) {
            throw_exception("parts have different sizes");
        }
    }
}

}  // namespace logicsim::editable_circuit::examples