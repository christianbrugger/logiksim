#include "vocabulary/display_state.h"

#include <stdexcept>
#include <string>

namespace logicsim {

template <>
auto format(display_state_t state) -> std::string {
    switch (state) {
        using enum display_state_t;

        case normal:
            return "normal";

        case valid:
            return "valid";
        case colliding:
            return "colliding";

        case temporary:
            return "temporary";
    }
    throw std::runtime_error("Don't know how to convert display_state_t to string.");
}

auto is_inserted(display_state_t display_state) -> bool {
    return display_state == display_state_t::normal ||
           display_state == display_state_t::valid;
}

}  // namespace logicsim
