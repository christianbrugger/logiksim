#include "vocabulary/connection_count.h"

namespace logicsim {

auto connection_count_t::format() const -> std::string {
    return fmt::format("{}", value);
}

auto connection_count_t::operator<=>(const connection_id_t& other) const
    -> std::strong_ordering {
    if (!other) {
        throw std::runtime_error("only valid ids can be compared");
    }
    const auto id_converted = gsl::narrow<connection_count_t::value_type>(other.value);
    return value <=> id_converted;
}

}  // namespace logicsim
