#include "random/schematic.h"

#include "algorithm/shuffle.h"
#include "algorithm/uniform_int_distribution.h"
#include "logic_item/schematic_info.h"
#include "schematic_old.h"

#include <stdexcept>

namespace logicsim {

namespace details {

auto add_random_element(Rng &rng, SchematicOld &schematic) -> void {
    static constexpr auto max_connections = 8;
    auto connection_dist = uint_distribution(1, max_connections);
    auto element_dist = uint_distribution<int8_t>(0, 2);

    const auto element_type =
        element_dist(rng) == 0
            ? ElementType::xor_element
            : (element_dist(rng) == 1 ? ElementType::buffer_element : ElementType::wire);

    const auto input_count =
        element_type == ElementType::xor_element ? connection_dist(rng) : 1;

    const auto output_count =
        element_type == ElementType::wire ? connection_dist(rng) : 1;

    // TODO remove narrow
    schematic.add_element(SchematicOld::ElementData {
        .element_type = element_type,
        .input_count =
            connection_count_t {gsl::narrow<connection_count_t::value_type>(input_count)},
        .output_count = connection_count_t {gsl::narrow<connection_count_t::value_type>(
            output_count)},
        .input_inverters = element_type == ElementType::buffer_element
                               ? logic_small_vector_t {true}
                               : logic_small_vector_t {},
        .output_delays =
            std::vector<delay_t>(output_count, element_output_delay(element_type)),
    });
}

auto create_random_elements(Rng &rng, SchematicOld &schematic, int n_elements) -> void {
    for (auto _ [[maybe_unused]] : range(n_elements)) {
        add_random_element(rng, schematic);
    }
}

auto create_random_connections(Rng &rng, SchematicOld &schematic, double connection_ratio)
    -> void {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw std::runtime_error("connection ratio needs to be between 0 and 1.");
    }

    // collect inputs
    std::vector<SchematicOld::Input> all_inputs;
    all_inputs.reserve(schematic.total_input_count());
    for (auto element : schematic.elements()) {
        for (auto input : element.inputs()) {
            all_inputs.push_back(input);
        }
    }

    // collect outputs
    std::vector<SchematicOld::Output> all_outputs;
    all_outputs.reserve(schematic.total_output_count());
    for (auto element : schematic.elements()) {
        for (auto output : element.outputs()) {
            all_outputs.push_back(output);
        }
    }

    shuffle(all_inputs, rng);
    shuffle(all_outputs, rng);

    auto n_max_connections =
        gsl::narrow<double>(std::min(std::size(all_inputs), std::size(all_outputs)));
    auto n_connections =
        gsl::narrow<std::size_t>(std::round(connection_ratio * n_max_connections));

    for (auto index : range(n_connections)) {
        all_inputs.at(index).connect(all_outputs.at(index));
    }
}
}  // namespace details

auto create_random_schematic(Rng &rng, int n_elements, double connection_ratio)
    -> SchematicOld {
    SchematicOld schematic;
    details::create_random_elements(rng, schematic, n_elements);
    details::create_random_connections(rng, schematic, connection_ratio);

    return schematic;
}

}  // namespace logicsim
