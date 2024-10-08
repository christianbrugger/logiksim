#include "random/random_schematic.h"

#include "algorithm/range.h"
#include "algorithm/round.h"
#include "algorithm/shuffle.h"
#include "algorithm/uniform_int_distribution.h"
#include "element/logicitem/schematic_info.h"
#include "schematic.h"

#include <exception>
#include <stdexcept>

namespace logicsim {

namespace details {

auto add_random_element(Rng &rng, Schematic &schematic) -> void {
    const auto element_dist = uint_distribution<int8_t>(0, 2);

    const auto element_type = [&] {
        switch (element_dist(rng)) {
            case 0:
                return ElementType::xor_element;
            case 1:
                return ElementType::buffer_element;
            case 2:
                return ElementType::wire;

            default:
                std::terminate();
        };
    }();

    constexpr static auto max_connections = 8;

    const auto input_count = [&] {
        switch (element_type) {
            using enum ElementType;

            case xor_element:
                return uint_distribution(2, max_connections)(rng);
            case buffer_element:
                return 1;
            case wire:
                return 1;

            default:
                std::terminate();
        };
    }();

    const auto output_count = [&] {
        switch (element_type) {
            using enum ElementType;

            case xor_element:
                return 1;
            case buffer_element:
                return 1;
            case wire:
                return uint_distribution(1, max_connections)(rng);

            default:
                std::terminate();
        };
    }();

    const auto delay = element_type == ElementType::wire
                           ? element_output_delay(LogicItemType::and_element)
                           : element_output_delay(to_logicitem_type(element_type));

    schematic.add_element(schematic::NewElement {
        .element_type = element_type,
        .input_count = connection_count_t {input_count},
        .output_count = connection_count_t {output_count},
        .input_inverters = element_type == ElementType::buffer_element
                               ? logic_small_vector_t {true}
                               : logic_small_vector_t(input_count, false),
        .output_delays = output_delays_t(output_count, delay),
    });
}

auto create_random_elements(Rng &rng, Schematic &schematic, int n_elements) -> void {
    for (auto _ [[maybe_unused]] : range(n_elements)) {
        add_random_element(rng, schematic);
    }
}

auto create_random_connections(Rng &rng, Schematic &schematic,
                               double connection_ratio) -> void {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw std::runtime_error("connection_ratio needs to be between 0 and 1.");
    }

    // collect inputs
    std::vector<input_t> all_inputs;
    all_inputs.reserve(schematic.total_input_count());
    for (auto element_id : element_ids(schematic)) {
        std::ranges::copy(inputs(schematic, element_id), std::back_inserter(all_inputs));
    }

    // collect outputs
    std::vector<output_t> all_outputs;
    all_outputs.reserve(schematic.total_output_count());
    for (auto element_id : element_ids(schematic)) {
        std::ranges::copy(outputs(schematic, element_id),
                          std::back_inserter(all_outputs));
    }

    shuffle(all_inputs, rng);
    shuffle(all_outputs, rng);

    const auto min_size = std::min(all_inputs.size(), all_outputs.size());
    const auto n_connections =
        round_to<std::size_t>(connection_ratio * gsl::narrow<double>(min_size));

    for (const auto index : range(n_connections)) {
        schematic.connect(all_inputs.at(index), all_outputs.at(index));
    }
}
}  // namespace details

auto create_random_schematic(Rng &rng, int n_elements,
                             double connection_ratio) -> Schematic {
    Schematic schematic;
    details::create_random_elements(rng, schematic, n_elements);
    details::create_random_connections(rng, schematic, connection_ratio);

    return schematic;
}

namespace {

auto random_output_delays(Rng &rng, connection_count_t count) -> output_delays_t {
    const auto delay_dist = uint_distribution<delay_t::rep>(5, 500);

    auto result = output_delays_t {};
    result.reserve(count.count());

    for (auto _ [[maybe_unused]] : range(count)) {
        result.push_back(delay_t {1us * delay_dist(rng)});
    }

    return result;
}

}  // namespace

auto with_custom_delays(Rng &rng, const Schematic &schematic_orig) -> Schematic {
    auto schematic = Schematic {};
    schematic.reserve(schematic_orig.size());

    for (const auto element_id : element_ids(schematic_orig)) {
        const auto element_type = schematic_orig.element_type(element_id);
        const auto output_count = schematic_orig.output_count(element_id);

        auto output_delays = random_output_delays(rng, output_count);
        const auto history_length = element_type == ElementType::wire
                                        ? output_delays.at(0) * 10
                                        : schematic_orig.history_length(element_id);

        schematic.add_element(schematic::NewElement {
            .element_type = element_type,
            .input_count = schematic_orig.input_count(element_id),
            .output_count = output_count,

            .sub_circuit_id = schematic_orig.sub_circuit_id(element_id),
            .input_inverters = schematic_orig.input_inverters(element_id),
            .output_delays = std::move(output_delays),
            .history_length = history_length,
        });
    }

    for (const auto element_id : element_ids(schematic_orig)) {
        for (const auto input : inputs(schematic_orig, element_id)) {
            if (const auto output = schematic_orig.output(input)) {
                schematic.connect(input, output);
            }
        }
    }

    return schematic;
}

}  // namespace logicsim
