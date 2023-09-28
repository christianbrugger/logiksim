#include "vocabulary/connection_count.h"

#include <gsl/gsl>

namespace logicsim {

auto connection_count_t::format() const -> std::string {
    return fmt::format("{}", count());
}

auto connection_count_t::operator<=>(const connection_id_t& other) const
    -> std::strong_ordering {
    if (!other) {
        throw std::runtime_error("only valid ids can be compared");
    }
    const auto id_converted = connection_count_t {other.value};
    return *this <=> id_converted;
}

auto first_connection_id(connection_count_t count) -> connection_id_t {
    if (count == connection_count_t {0}) {
        throw std::runtime_error("has no first id");
    }

    return connection_id_t {0};
}

auto last_connection_id(connection_count_t count) -> connection_id_t {
    if (count == connection_count_t {0}) {
        throw std::runtime_error("has no last id");
    }
    const auto value = (count - connection_count_t {1}).count();
    return connection_id_t {gsl::narrow<connection_id_t::value_type>(value)};
}

}  // namespace logicsim
