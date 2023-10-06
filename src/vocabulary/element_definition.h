#ifndef LOGICSIM_VOCABULARY_ELEMENT_DEFINITION_H
#define LOGICSIM_VOCABULARY_ELEMENT_DEFINITION_H

#include "format/struct.h"
#include "vocabulary/circuit_id.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/delay.h"
#include "vocabulary/element_type.h"
#include "vocabulary/logic_small_vector.h"
#include "vocabulary/orientation.h"

#include <compare>
#include <optional>
#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Clock generator specific attributes.
 */
struct attributes_clock_generator_t {
    std::string name {"clock"};

    // all times are for half the clock period
    delay_t time_symmetric {500us};
    delay_t time_on {500us};
    delay_t time_off {500us};

    bool is_symmetric {true};
    bool show_simulation_controls {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_period() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const attributes_clock_generator_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const attributes_clock_generator_t&) const = default;
};

static_assert(std::is_aggregate_v<attributes_clock_generator_t>);

/**
 * @brief: Defines all attributes of an circuit element.
 */
struct ElementDefinition {
    ElementType element_type {ElementType::unused};
    connection_count_t input_count {0};
    connection_count_t output_count {0};
    orientation_t orientation {orientation_t::undirected};

    circuit_id_t circuit_id {null_circuit};  // TODO rename to sub_circuit_id ?
    logic_small_vector_t input_inverters {};
    logic_small_vector_t output_inverters {};

    std::optional<attributes_clock_generator_t> attrs_clock_generator {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const ElementDefinition& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const ElementDefinition& other) const = default;
};

static_assert(std::is_aggregate_v<ElementDefinition>);

}  // namespace logicsim

#endif
