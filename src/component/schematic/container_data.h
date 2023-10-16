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
using connection_vector_t = folly::small_vector<connection_t, connection_vector_size>;
static_assert(sizeof(connection_t) == 8);
static_assert(sizeof(connection_vector_t) == 32);
}  // namespace detail

class ContainerData {
   public:
    using connection_vector_t = schematic::detail::connection_vector_t;

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

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2,
                           bool update_connections) -> void;
    auto update_swapped_connections(element_id_t new_element_id,
                                    element_id_t old_element_id) -> void;
    auto delete_last_element(bool clear_connections) -> void;
    [[nodiscard]] auto last_element_id() const -> element_id_t;

   private:
    std::vector<ElementType> element_types_ {};
    std::vector<circuit_id_t> sub_circuit_ids_ {};
    std::vector<connection_vector_t> input_connections_ {};
    std::vector<connection_vector_t> output_connections_ {};
    std::vector<logic_small_vector_t> input_inverters_ {};
    std::vector<output_delays_t> output_delays_ {};
    std::vector<delay_t> history_lengths_ {};

    std::size_t total_input_count_ {0};
    std::size_t total_output_count_ {0};
};

auto swap(ContainerData &a, ContainerData &b) noexcept -> void;

}  // namespace schematic

}  // namespace logicsim

template <>
auto std::swap(logicsim::schematic::ContainerData &a,
               logicsim::schematic::ContainerData &b) noexcept -> void;

#endif
