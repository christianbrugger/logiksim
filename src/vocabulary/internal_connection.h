#ifndef LOGICSIM_VOCABULARY_INTERNAL_CONNECTION_H
#define LOGICSIM_VOCABULARY_INTERNAL_CONNECTION_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"

#include <compare>

namespace logicsim {

struct internal_connection_t {
    connection_id_t output;
    connection_id_t input;

    [[nodiscard]] auto format() -> std::string;

    [[nodiscard]] auto operator==(const internal_connection_t&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const internal_connection_t&) const = default;
};

}  // namespace logicsim

#endif
