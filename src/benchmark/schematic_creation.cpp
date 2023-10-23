#include "benchmark/schematic_creation.h"

#include "algorithm/range.h"
#include "logic_item/schematic_info.h"
#include "schematic.h"

namespace logicsim {

auto benchmark_schematic(const int n_elements) -> Schematic {
    Schematic schematic {};

    const auto wire_delay = element_output_delay(ElementType::and_element);

    auto elem0 = schematic.add_element(schematic::NewElement {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .input_inverters = {false, false},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    for ([[maybe_unused]] auto count : range(n_elements - 1)) {
        const auto wire0 = schematic.add_element(schematic::NewElement {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {1},
            .output_count = connection_count_t {2},
            .input_inverters = {false},
            .output_delays = {wire_delay, wire_delay},
        });
        const auto elem1 = schematic.add_element(schematic::NewElement {
            .element_type = ElementType::and_element,
            .input_count = connection_count_t {2},
            .output_count = connection_count_t {1},
            .input_inverters = {false, false},
            .output_delays = {element_output_delay(ElementType::and_element)},
        });

        constexpr static auto id_0 = connection_id_t {0};
        constexpr static auto id_1 = connection_id_t {1};

        schematic.connect(output_t {elem0, id_0}, input_t {wire0, id_0});

        schematic.connect(output_t {wire0, id_0}, input_t {elem1, id_0});
        schematic.connect(output_t {wire0, id_1}, input_t {elem1, id_1});

        elem0 = elem1;
    }

    return schematic;
}

}  // namespace logicsim
