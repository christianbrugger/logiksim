#include "benchmark/schematic_creation.h"

#include "algorithm/range.h"
#include "algorithm/shuffle.h"
#include "algorithm/uniform_int_distribution.h"
#include "logic_item/schematic_info.h"
#include "schematic_old.h"

#include <stdexcept>

namespace logicsim {

auto benchmark_schematic(const int n_elements) -> SchematicOld {
    SchematicOld schematic {};

    auto wire_delay = element_output_delay(ElementType::and_element);

    auto elem0 = schematic.add_element(SchematicOld::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {element_output_delay(ElementType::and_element)},
    });

    for ([[maybe_unused]] auto count : range(n_elements - 1)) {
        auto wire0 = schematic.add_element(SchematicOld::ElementData {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {1},
            .output_count = connection_count_t {2},
            .output_delays = {wire_delay, wire_delay},
        });
        auto elem1 = schematic.add_element(SchematicOld::ElementData {
            .element_type = ElementType::and_element,
            .input_count = connection_count_t {2},
            .output_count = connection_count_t {1},
            .output_delays = {element_output_delay(ElementType::and_element)},
        });

        elem0.output(connection_id_t {0}).connect(wire0.input(connection_id_t {0}));

        wire0.output(connection_id_t {0}).connect(elem1.input(connection_id_t {0}));
        wire0.output(connection_id_t {1}).connect(elem1.input(connection_id_t {1}));

        elem0 = elem1;
    }

    return schematic;
}

}  // namespace logicsim
