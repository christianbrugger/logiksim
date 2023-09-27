#ifndef LOGICSIM_VOCABULARY_CIRCUIT_ID_H
#define LOGICSIM_VOCABULARY_CIRCUIT_ID_H

#include "format/struct.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies a specefic circuit or sub-circuit.
 */
struct circuit_id_t {
    using value_type = int16_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto operator==(const circuit_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const circuit_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> circuit_id_t;
};

static_assert(std::is_aggregate_v<circuit_id_t>);
static_assert(std::is_trivial_v<circuit_id_t>);

constexpr inline auto null_circuit = circuit_id_t {-1};

//
// Implementation
//

constexpr circuit_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto circuit_id_t::max() noexcept -> circuit_id_t {
    return circuit_id_t {std::numeric_limits<value_type>::max()};
};

}  // namespace logicsim

#endif
