#include "vocabulary/insertion_mode.h"

#include <stdexcept>
#include <string>

namespace logicsim {

template <>
auto format(InsertionMode mode) -> std::string {
    switch (mode) {
        using enum InsertionMode;

        case insert_or_discard:
            return "insert_or_discard";
        case collisions:
            return "collisions";
        case temporary:
            return "temporary";
    }
    throw std::runtime_error("Don't know how to convert insertion mode to string.");
}

auto to_insertion_mode(display_state_t display_state) -> InsertionMode {
    switch (display_state) {
        using enum display_state_t;

        case normal:
            return InsertionMode::insert_or_discard;
        case colliding:
            return InsertionMode::collisions;
        case valid:
            return InsertionMode::collisions;
        case temporary:
            return InsertionMode::temporary;
    };

    throw std::runtime_error("Unknown display state.");
};

}  // namespace logicsim
