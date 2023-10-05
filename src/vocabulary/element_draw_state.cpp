#include "vocabulary/element_draw_state.h"

#include <exception>

namespace logicsim {

template <>
auto format(ElementDrawState state) -> std::string {
    switch (state) {
        using enum ElementDrawState;

        case normal:
            return "normal";
        case normal_selected:
            return "normal_selected";
        case valid:
            return "valid";
        case simulated:
            return "simulated";

        case colliding:
            return "colliding";
        case temporary_selected:
            return "temporary_selected";
    }

    std::terminate();
}

auto is_inserted(ElementDrawState state) noexcept -> bool {
    using enum ElementDrawState;

    return state == normal || state == normal_selected || state == valid ||
           state == simulated;
}

auto has_overlay(ElementDrawState state) noexcept -> bool {
    using enum ElementDrawState;

    return state == normal_selected || state == valid || state == colliding ||
           state == temporary_selected;
}

}  // namespace logicsim
