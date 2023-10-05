#include "benchmark/schematic_creation.h"

#include "algorithm/range.h"
#include "algorithm/shuffle.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <stdexcept>

namespace logicsim {

auto benchmark_schematic(const int n_elements) -> Schematic {
    Schematic schematic {};

    auto elem0 = schematic.add_element(Schematic::ElementData {
        .element_type = ElementType::and_element,
        .input_count = connection_count_t {2},
        .output_count = connection_count_t {1},
        .output_delays = {defaults::logic_item_delay},
    });

    for ([[maybe_unused]] auto count : range(n_elements - 1)) {
        auto wire0 = schematic.add_element(Schematic::ElementData {
            .element_type = ElementType::wire,
            .input_count = connection_count_t {1},
            .output_count = connection_count_t {2},
            .output_delays = {defaults::logic_item_delay, defaults::logic_item_delay},
        });
        auto elem1 = schematic.add_element(Schematic::ElementData {
            .element_type = ElementType::and_element,
            .input_count = connection_count_t {2},
            .output_count = connection_count_t {1},
            .output_delays = {defaults::logic_item_delay},
        });

        elem0.output(connection_id_t {0}).connect(wire0.input(connection_id_t {0}));

        wire0.output(connection_id_t {0}).connect(elem1.input(connection_id_t {0}));
        wire0.output(connection_id_t {1}).connect(elem1.input(connection_id_t {1}));

        elem0 = elem1;
    }

    return schematic;
}

namespace details {

template <std::uniform_random_bit_generator G>
void add_random_element(Schematic &schematic, G &rng) {
    static constexpr auto max_connections = 8;
    auto connection_dist =
        boost::random::uniform_int_distribution<int> {1, max_connections};
    auto element_dist = boost::random::uniform_int_distribution<int8_t> {0, 2};

    const auto element_type =
        element_dist(rng) == 0
            ? ElementType::xor_element
            : (element_dist(rng) == 1 ? ElementType::buffer_element : ElementType::wire);

    const auto input_count =
        element_type == ElementType::xor_element ? connection_dist(rng) : 1;

    const auto output_count =
        element_type == ElementType::wire ? connection_dist(rng) : 1;

    // TODO remove narrow
    schematic.add_element(Schematic::ElementData {
        .element_type = element_type,
        .input_count =
            connection_count_t {gsl::narrow<connection_count_t::value_type>(input_count)},
        .output_count = connection_count_t {gsl::narrow<connection_count_t::value_type>(
            output_count)},
        .input_inverters = element_type == ElementType::buffer_element
                               ? logic_small_vector_t {true}
                               : logic_small_vector_t {},
        .output_delays = std::vector<delay_t>(output_count, defaults::logic_item_delay),
    });
}

template <std::uniform_random_bit_generator G>
void create_random_elements(Schematic &schematic, G &rng, int n_elements) {
    for (auto _ [[maybe_unused]] : range(n_elements)) {
        add_random_element(schematic, rng);
    }
}

template <std::uniform_random_bit_generator G>
void create_random_connections(Schematic &schematic, G &rng, double connection_ratio) {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw std::runtime_error("connection ratio needs to be between 0 and 1.");
    }

    // collect inputs
    std::vector<Schematic::Input> all_inputs;
    all_inputs.reserve(schematic.total_input_count());
    for (auto element : schematic.elements()) {
        for (auto input : element.inputs()) {
            all_inputs.push_back(input);
        }
    }

    // collect outputs
    std::vector<Schematic::Output> all_outputs;
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

template <std::uniform_random_bit_generator G>
auto create_random_schematic(G &rng, int n_elements, double connection_ratio)
    -> Schematic {
    Schematic schematic;
    details::create_random_elements(schematic, rng, n_elements);
    details::create_random_connections(schematic, rng, connection_ratio);

    return schematic;
}

// template instatiations

template auto create_random_schematic(boost::random::mt19937 &rng, int n_elements,
                                      double connection_ratio) -> Schematic;

}  // namespace logicsim