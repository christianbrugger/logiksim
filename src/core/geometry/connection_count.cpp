#include "core/geometry/connection_count.h"

namespace logicsim {

auto first_id(connection_count_t count) -> connection_id_t {
    if (count == connection_count_t {0}) {
        throw std::runtime_error("has no first id");
    }

    return connection_id_t {0};
}

auto last_id(connection_count_t count) -> connection_id_t {
    if (count == connection_count_t {0}) {
        throw std::runtime_error("has no last id");
    }
    const auto value = (count - connection_count_t {1}).count();
    return connection_id_t {gsl::narrow_cast<connection_id_t::value_type>(value)};
}

auto id_range(connection_count_t count) -> range_extended_t<connection_id_t> {
    return range_extended<connection_id_t>(std::size_t {count.count()});
}

}  // namespace logicsim
