#ifndef LOGICSIM_VOCABULARY_CIRCUIT_ID_H
#define LOGICSIM_VOCABULARY_CIRCUIT_ID_H

#include "format/struct.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <string>

namespace logicsim {

/**
 * @brief: Identifies a specefic circuit or sub-circuit.
 */
struct circuit_id_t {
    using value_type = int16_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const circuit_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const circuit_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }
};

static_assert(std::is_trivial<circuit_id_t>::value);

constexpr inline auto null_circuit = circuit_id_t {-1};

}  // namespace logicsim

#endif
