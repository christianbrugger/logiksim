#ifndef LOGICSIM_VOCABULARY_LOGICITEM_CONNECTION_H
#define LOGICSIM_VOCABULARY_LOGICITEM_CONNECTION_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/orientation.h"

#include <compare>
#include <string>

namespace logicsim {

/**
 * @brief: Identifies an input or output of a specific circuit element.
 *
 * Class invariants:
 *     * element_id and connection_id are either both valid or null.
 */
struct logicitem_connection_t {
    logicitem_id_t logicitem_id {null_logicitem_id};
    connection_id_t connection_id {null_connection_id};
    orientation_t orientation {orientation_t::undirected};

    [[nodiscard]] explicit constexpr logicitem_connection_t() noexcept = default;
    [[nodiscard]] explicit constexpr logicitem_connection_t(
        logicitem_id_t logicitem_id_, connection_id_t connection_id_,
        orientation_t orientation_);

    /**
     * @brief: The bool cast tests if this connection is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const logicitem_connection_t &other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const logicitem_connection_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<logicitem_connection_t>);
static_assert(std::is_trivially_copy_assignable_v<logicitem_connection_t>);

//
// Implemention
//

constexpr logicitem_connection_t::logicitem_connection_t(logicitem_id_t logicitem_id_,
                                                         connection_id_t connection_id_,
                                                         orientation_t orientation_)
    : logicitem_id {logicitem_id_},
      connection_id {connection_id_},
      orientation {orientation_} {
    if (bool {logicitem_id} != bool {connection_id}) [[unlikely]] {
        throw std::runtime_error("Connection cannot be partially null.");
    }
}

constexpr logicitem_connection_t::operator bool() const noexcept {
    return bool {logicitem_id};
}

}  // namespace logicsim

#endif
