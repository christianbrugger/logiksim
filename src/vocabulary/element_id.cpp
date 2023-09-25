#include "vocabulary/element_id.h"

namespace logicsim {

auto element_id_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}
