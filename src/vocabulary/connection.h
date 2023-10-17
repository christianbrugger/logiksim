#ifndef LOGICSIM_VOCABULARY_CONNECTION_H
#define LOGICSIM_VOCABULARY_CONNECTION_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/element_id.h"

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies an input or output of a specific circuit element.
 */
struct connection_t {
    element_id_t element_id {null_element};
    connection_id_t connection_id {null_connection_id};

    [[nodiscard]] explicit constexpr connection_t() noexcept = default;
    [[nodiscard]] explicit constexpr connection_t(element_id_t element_id_,
                                                  connection_id_t connection_id_);

    /**
     * @brief: The bool cast tests if this segment is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<connection_t>);
static_assert(std::is_trivially_copy_assignable_v<connection_t>);

constexpr inline auto null_connection = connection_t {};

//
// Implemention
//

constexpr connection_t::connection_t(element_id_t element_id_,
                                     connection_id_t connection_id_)
    : element_id {element_id_}, connection_id {connection_id_} {
    if (bool {element_id} != bool {connection_id}) [[unlikely]] {
        throw std::runtime_error("Connection cannot be partially null.");
    }
}

constexpr connection_t::operator bool() const noexcept {
    return bool {element_id};
}

//
// Implemention
//

}  // namespace logicsim

#endif
