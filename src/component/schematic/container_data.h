#ifndef LOGICSIM_COMPONENTS_SCHEMATIC_CONTAINER_DATA_H
#define LOGICSIM_COMPONENTS_SCHEMATIC_CONTAINER_DATA_H

#include "vocabulary/circuit_id.h"
#include "vocabulary/connection.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/delay.h"
#include "vocabulary/element_type.h"
#include "vocabulary/logic_small_vector.h"
#include "vocabulary/output_delays.h"

#include <folly/small_vector.h>

#include <utility>

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

/**
 * brief: Stores the schematic data.
 *
 * Class invariants:
 *   * all vectors have same size
 *   * connection point to a valid element_id / connection_id
 *   * forward and backward connections point to each other
 *   * total connection counts match sum of all input / output connections
 */
class ContainerData {
   public:
    using input_vector_t = schematic::detail::input_vector_t;
    using output_vector_t = schematic::detail::output_vector_t;

   public:
    auto swap(ContainerData &other) noexcept -> void;

    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto total_input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto total_output_count() const noexcept -> std::size_t;

    auto clear() -> void;
    auto add_element(schematic::NewElement &&data) -> element_id_t;
    auto swap_and_delete_element(element_id_t element_id) -> element_id_t;
    auto swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void;

    [[nodiscard]] auto connection(input_t input) const -> output_t;
    [[nodiscard]] auto connection(output_t output) const -> input_t;
    auto connect(input_t input, output_t output) -> void;
    auto connect(output_t output, input_t input) -> void;
    auto clear(input_t input) -> void;
    auto clear(output_t output) -> void;
    auto clear_all(element_id_t element_id) -> void;

    [[nodiscard]] auto input_count(element_id_t element_id) const -> connection_count_t;
    [[nodiscard]] auto output_count(element_id_t element_id) const -> connection_count_t;

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2) -> void;
    auto update_swapped_connections(element_id_t new_element_id,
                                    element_id_t old_element_id) -> void;
    auto delete_last_unconnected_element() -> void;

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

auto swap(ContainerData &a, ContainerData &b) noexcept -> void;

[[nodiscard]] auto has_input_connections(const ContainerData &data,
                                         element_id_t element_id) -> bool;
[[nodiscard]] auto has_output_connections(const ContainerData &data,
                                          element_id_t element_id) -> bool;

}  // namespace schematic

}  // namespace logicsim

template <>
auto std::swap(logicsim::schematic::ContainerData &a,
               logicsim::schematic::ContainerData &b) noexcept -> void;

#endif
