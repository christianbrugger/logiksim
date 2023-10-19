#include "vocabulary/connection_count.h"

#include "logging.h"  // TODO remove

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

}  // namespace logicsim
