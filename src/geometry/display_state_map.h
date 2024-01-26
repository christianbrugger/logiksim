#ifndef LOGICSIM_GEOMETRY_DISPLAY_STATE_MAP_H
#define LOGICSIM_GEOMETRY_DISPLAY_STATE_MAP_H

#include "vocabulary/display_state_map.h"
#include "vocabulary/insertion_mode.h"

namespace logicsim {

[[nodiscard]] auto found_states_matches_insertion_mode(DisplayStateMap found_states,
                                                       InsertionMode insertion_mode)
    -> bool;

}

#endif
