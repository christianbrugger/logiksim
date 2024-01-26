#include "geometry/display_state_map.h"

#include <algorithm>

namespace logicsim {

auto count_values(const DisplayStateMap& states) -> std::size_t {
    return std::ranges::count_if(all_display_states,
                                 [&](display_state_t state) { return states.at(state); });
}

auto found_states_matches_insertion_mode(const DisplayStateMap& found_states,
                                         InsertionMode insertion_mode) -> bool {
    return std::ranges::none_of(all_display_states, [&](const auto display_state) {
        return found_states.at(display_state) &&
               to_insertion_mode(display_state) != insertion_mode;
    });
}

}  // namespace logicsim
