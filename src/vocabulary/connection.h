#ifndef LOGICSIM_VOCABULARY_CONNECTION_H
#define LOGICSIM_VOCABULARY_CONNECTION_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/element_id.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies an input or output of a specific circuit element.
 */
struct connection_t {
    element_id_t element_id;
    connection_id_t connection_id;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_t &other) const = default;
};

static_assert(std::is_aggregate_v<connection_t>);

}  // namespace logicsim

#endif
