#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_ALPHA_VALUES_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_ALPHA_VALUES_H

#include "core/vocabulary/color.h"
#include "core/vocabulary/element_draw_state.h"

namespace logicsim {

namespace defaults {
namespace element_state_alpha {
constexpr static inline auto normal = color_t::value_type {255};
constexpr static inline auto colliding = color_t::value_type {64};
constexpr static inline auto temporary = color_t::value_type {128};
}  // namespace element_state_alpha
}  // namespace defaults

constexpr auto state_alpha(ElementDrawState state) noexcept -> color_t::value_type {
    switch (state) {
        using enum ElementDrawState;

        case normal:
        case normal_selected:
        case valid:
        case simulated:
            return defaults::element_state_alpha::normal;

        case colliding:
            return defaults::element_state_alpha::colliding;
        case temporary_selected:
            return defaults::element_state_alpha::temporary;
    }
    std::terminate();
}

consteval auto with_alpha(color_t color, ElementDrawState state) noexcept -> color_t {
    return color_t {color.r(), color.g(), color.b(), state_alpha(state)};
}

constexpr auto with_alpha_runtime(color_t color, ElementDrawState state) noexcept
    -> color_t {
    return color_t {color.r(), color.g(), color.b(), state_alpha(state)};
}

}  // namespace logicsim

#endif
