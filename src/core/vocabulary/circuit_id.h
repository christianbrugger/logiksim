#ifndef LOGICSIM_VOCABULARY_CIRCUIT_ID_H
#define LOGICSIM_VOCABULARY_CIRCUIT_ID_H

#include "core/algorithm/narrow_integral.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies a specific circuit or sub-circuit.
 */
struct circuit_id_t {
    using value_type = int16_t;
    // we expose the value, as the type has no invariant
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr circuit_id_t() = default;
    [[nodiscard]] explicit constexpr circuit_id_t(integral auto value_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto operator==(const circuit_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const circuit_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> circuit_id_t;
};

static_assert(std::is_trivially_copyable_v<circuit_id_t>);
static_assert(std::is_trivially_copy_assignable_v<circuit_id_t>);

//
// Implementation
//

constexpr circuit_id_t::circuit_id_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr circuit_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto circuit_id_t::max() noexcept -> circuit_id_t {
    return circuit_id_t {std::numeric_limits<value_type>::max()};
};

//
// Constants
//

constexpr inline auto null_circuit = circuit_id_t {-1};
static_assert(null_circuit == circuit_id_t {});

}  // namespace logicsim

#endif
