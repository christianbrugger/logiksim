#include "setting_handle.h"

#include "layout.h"
#include "layout_calculation.h"

namespace logicsim {
auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> setting_handle_t {
    const auto element = layout.element(element_id);

    switch (layout.element_type(element_id)) {
        using enum ElementType;

        case clock_generator: {
            return setting_handle_t {transform(element.position(), element.orientation(),
                                               point_fine_t {1.0, 1.0})};
        }

        case unused:
        case placeholder:
        case wire:

        case buffer_element:
        case and_element:
        case or_element:
        case xor_element:

        case button:
        case led:
        case display_number:
        case display_ascii:

        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:

        case sub_circuit:
            return {};
    };

    throw_exception("unknown ElementType in setting_handle_position");
}

auto setting_handle_positions(const Layout& layout, const Selection& selection)
    -> setting_handle_t {
    return setting_handle_t();
}

auto size_handle_rect_px(setting_handle_t handle, const ViewConfig& config) -> BLRect {
    return BLRect();
}

auto size_handle_rect_gird(setting_handle_t handle, const ViewConfig& config)
    -> rect_fine_t {
    return rect_fine_t();
}

auto get_colliding_settings_handle(point_fine_t position, const Layout& layout,
                                   const Selection& selection, const ViewConfig& config)
    -> std::optional<setting_handle_t> {
    return std::optional<setting_handle_t>();
}

//
}  // namespace logicsim