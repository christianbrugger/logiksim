#ifndef LOGICSIM_SCHEMATIC_H
#define LOGICSIM_SCHEMATIC_H

#include "algorithm/range_extended.h"
#include "format/container.h"
#include "format/struct.h"
#include "geometry/connection.h"
#include "vocabulary/circuit_id.h"
#include "vocabulary/connection.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/delay.h"
#include "vocabulary/element_id.h"
#include "vocabulary/element_type.h"
#include "vocabulary/logic_small_vector.h"
#include "vocabulary/output_delays.h"

#include <folly/small_vector.h>

#include <utility>  // std::swap

namespace logicsim {

namespace schematic {

namespace defaults {
constexpr static inline auto no_history = delay_t {0ns};
}

struct NewElement {
    ElementType element_type {ElementType::unused};
    connection_count_t input_count {0};
    connection_count_t output_count {0};

    circuit_id_t sub_circuit_id {null_circuit};
    logic_small_vector_t input_inverters {};
    output_delays_t output_delays {};
    delay_t history_length = defaults::no_history;
};

namespace detail {
constexpr inline auto connection_vector_size = 3;
using input_vector_t = folly::small_vector<output_t, connection_vector_size>;
using output_vector_t = folly::small_vector<input_t, connection_vector_size>;

static_assert(sizeof(connection_t) == 8);
static_assert(sizeof(input_vector_t) == 32);
static_assert(sizeof(output_vector_t) == 32);
}  // namespace detail

}  // namespace schematic

/**
 * brief: Stores the schematic data.
 *
 * Class invariants:
 *      + all vectors have same size
 *      + all connections point to a valid element_id / connection_id
 *      + forward and backward connections point to each other
 *      + total connection counts match sum of all input / output connections
 *      + output_delays need to be positive
 *      + history_lengths need to be zero or positive
 *      + element input & output counts are valid according to layout_info
 */
class Schematic {
   public:
    using input_vector_t = schematic::detail::input_vector_t;
    using output_vector_t = schematic::detail::output_vector_t;

   public:
    auto swap(Schematic &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const Schematic &) const -> bool = default;

    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto total_input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto total_output_count() const noexcept -> std::size_t;

    auto clear() -> void;
    auto reserve(std::size_t new_capacity) -> void;
    auto shrink_to_fit() -> void;
    auto add_element(schematic::NewElement &&data) -> element_id_t;

    // connections
    [[nodiscard]] auto input_count(element_id_t element_id) const -> connection_count_t;
    [[nodiscard]] auto output_count(element_id_t element_id) const -> connection_count_t;
    [[nodiscard]] auto output(input_t input) const -> output_t;
    [[nodiscard]] auto input(output_t output) const -> input_t;
    auto connect(input_t input, output_t output) -> void;
    auto connect(output_t output, input_t input) -> void;
    auto clear(input_t input) -> void;
    auto clear(output_t output) -> void;
    auto clear_all_connections(element_id_t element_id) -> void;

    // attributes for element_id
    [[nodiscard]] auto element_type(element_id_t element_id) const -> ElementType;
    [[nodiscard]] auto sub_circuit_id(element_id_t element_id) const -> circuit_id_t;
    [[nodiscard]] auto input_inverters(element_id_t element_id) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto output_delays(element_id_t element_id) const
        -> const output_delays_t &;
    [[nodiscard]] auto history_length(element_id_t element_id) const -> delay_t;

    // attributes for single input / output
    [[nodiscard]] auto output_delay(output_t output) const -> delay_t;
    [[nodiscard]] auto input_inverted(input_t input) const -> bool;
    auto set_input_inverter(input_t input, bool value) -> void;

   private:
    auto clear_connection(input_t input, output_t output) -> void;
    [[nodiscard]] auto last_element_id() const -> element_id_t;

   private:
    std::vector<ElementType> element_types_ {};
    std::vector<circuit_id_t> sub_circuit_ids_ {};
    std::vector<input_vector_t> input_connections_ {};
    std::vector<output_vector_t> output_connections_ {};
    std::vector<logic_small_vector_t> input_inverters_ {};
    std::vector<output_delays_t> output_delays_ {};
    std::vector<delay_t> history_lengths_ {};

    std::size_t total_input_count_ {0};
    std::size_t total_output_count_ {0};
};

auto swap(Schematic &a, Schematic &b) noexcept -> void;

[[nodiscard]] auto has_input_connections(const Schematic &data,
                                         element_id_t element_id) -> bool;
[[nodiscard]] auto has_output_connections(const Schematic &data,
                                          element_id_t element_id) -> bool;

//
// Iteration
//

[[nodiscard]] auto element_ids(const Schematic &schematic)
    -> range_extended_t<element_id_t>;

[[nodiscard]] inline auto inputs(const Schematic &schematic, element_id_t element_id) {
    return inputs(element_id, schematic.input_count(element_id));
}

[[nodiscard]] auto input_ids(const Schematic &schematic, element_id_t element_id)
    -> range_extended_t<connection_id_t>;

[[nodiscard]] inline auto outputs(const Schematic &schematic, element_id_t element_id) {
    return outputs(element_id, schematic.output_count(element_id));
}

[[nodiscard]] auto output_ids(const Schematic &schematic, element_id_t element_id)
    -> range_extended_t<connection_id_t>;

//
// Formatting
//

[[nodiscard]] auto format_element(const Schematic &schematic,
                                  element_id_t element) -> std::string;
[[nodiscard]] auto format_element_with_connections(
    const Schematic &schematic, element_id_t element_id) -> std::string;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Schematic &a, logicsim::Schematic &b) noexcept -> void;

#endif
