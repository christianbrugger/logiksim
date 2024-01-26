#include "geometry/display_state_map.h"

#include <algorithm>

namespace logicsim {

auto found_states_matches_insertion_mode(DisplayStateMap found_states,
                                         InsertionMode insertion_mode) -> bool {
    return std::ranges::none_of(all_display_states, [&](const auto display_state) {
        return found_states.at(display_state) &&
               to_insertion_mode(display_state) != insertion_mode;
    });
}

}  // namespace logicsim
