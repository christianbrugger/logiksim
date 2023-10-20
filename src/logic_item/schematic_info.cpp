#include "logic_item/schematic_info.h"

#include <exception>

namespace logicsim {

namespace {

constexpr static inline auto logic_item_delay = delay_t {3us};

constexpr static inline auto button_delay = delay_t::epsilon();
constexpr static inline auto clock_generator_output_delay = delay_t::epsilon();

}  // namespace

auto element_output_delay(ElementType element_type) -> delay_t {
    switch (element_type) { 
        using enum ElementType; 
    
        case button:
            return button_delay;
        case clock_generator:
            return clock_generator_output_delay;

        default:
            return logic_item_delay;
    };
    std::terminate();
}

}
