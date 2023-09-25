#ifndef LOGICSIM_VOCABULARY_CONNECTION_ID_H
#define LOGICSIM_VOCABULARY_CONNECTION_ID_H

#include "format/struct.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies an input or output of an unspecified circuit element.
 */
struct connection_id_t {
    using value_type = int8_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }

    constexpr auto operator++() noexcept -> connection_id_t & {
        ++value;
        return *this;
    }

    constexpr auto operator++(int) noexcept -> connection_id_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }
};

static_assert(std::is_trivial<connection_id_t>::value);

constexpr inline auto null_connection = connection_id_t {-1};

}  // namespace logicsim

#endif
