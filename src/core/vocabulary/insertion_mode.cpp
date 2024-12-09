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
auto format(SegmentInsertionHint hint) -> std::string {
    switch (hint) {
        using enum SegmentInsertionHint;

        case no_hint:
            return "no_hint";
        case assume_colliding:
            return "assume_colliding";
    }
    std::terminate();
}

auto segment_insertion_hint_valid(InsertionMode mode, SegmentInsertionHint hint) -> bool {
    if (hint == SegmentInsertionHint::assume_colliding &&
        !(mode == InsertionMode::collisions ||
          mode == InsertionMode::insert_or_discard)) {
        return false;
    }

    return true;
}

}  // namespace logicsim
