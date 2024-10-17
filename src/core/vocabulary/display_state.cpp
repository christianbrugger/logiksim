#include "core/vocabulary/display_state.h"

#include <exception>
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
    std::terminate();
}

auto is_inserted(display_state_t display_state) -> bool {
    return display_state == display_state_t::normal ||
           display_state == display_state_t::valid;
}

}  // namespace logicsim
