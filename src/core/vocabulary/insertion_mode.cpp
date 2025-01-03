#include "core/vocabulary/insertion_mode.h"

#include <exception>
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
    std::terminate();
}

auto to_insertion_mode(display_state_t display_state) -> InsertionMode {
    switch (display_state) {
        using enum display_state_t;

        case normal:
            return InsertionMode::insert_or_discard;
        case colliding:
        case valid:
            return InsertionMode::collisions;
        case temporary:
            return InsertionMode::temporary;
    };
    std::terminate();
};

template <>
auto format(InsertionHint hint) -> std::string {
    switch (hint) {
        using enum InsertionHint;

        case no_hint:
            return "no_hint";
        case assume_colliding:
            return "assume_colliding";
        case expect_valid:
            return "expect_valid";
    }
    std::terminate();
}

auto insertion_hint_valid(InsertionMode mode, InsertionHint hint) -> bool {
    return hint == InsertionHint::no_hint || (mode == InsertionMode::collisions ||
                                              mode == InsertionMode::insert_or_discard);
}

}  // namespace logicsim
