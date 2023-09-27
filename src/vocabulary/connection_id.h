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

    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto operator==(const connection_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept -> connection_id_t;
    [[nodiscard]] static constexpr auto max() noexcept -> connection_id_t;

    constexpr auto operator++() noexcept -> connection_id_t &;
    constexpr auto operator++(int) noexcept -> connection_id_t;
};

static_assert(std::is_trivial<connection_id_t>::value);

constexpr inline auto null_connection = connection_id_t {-1};

//
// Implementation
//

constexpr connection_id_t::operator bool() const noexcept {
    return value >= 0;
}

inline constexpr auto connection_id_t::min() noexcept -> connection_id_t {
    return connection_id_t {0};
}

constexpr auto connection_id_t::max() noexcept -> connection_id_t {
    return connection_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto connection_id_t::operator++() noexcept -> connection_id_t & {
    ++value;
    return *this;
}

constexpr auto connection_id_t::operator++(int) noexcept -> connection_id_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

}  // namespace logicsim

#endif
