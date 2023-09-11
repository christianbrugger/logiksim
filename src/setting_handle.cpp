#include "setting_handle.h"

#include "collision.h"
#include "editable_circuit/selection.h"
#include "layout.h"
#include "layout_calculation.h"
#include "scene.h"

namespace logicsim {
auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> std::optional<setting_handle_t> {
    const auto element = layout.element(element_id);

    switch (layout.element_type(element_id)) {
        using enum ElementType;

        case clock_generator: {
            constexpr auto overdraw = defaults::logic_item_body_overdraw;
            constexpr auto size = defaults::setting_handle_size;
            constexpr auto margin = defaults::setting_handle_margin;

            const auto width = 3.0;
            const auto height = 2.0;

            return setting_handle_t {
                .position = transform(element.position(), element.orientation(),
                                      point_fine_t {
                                          width - size / 2.0 - margin,
                                          height + overdraw - size / 2.0 - margin,
                                      }),
                .icon = icon_t::setting_handle_clock,
            };
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

namespace {

auto get_single_logic_item(const Selection& selection) -> element_id_t {
    if (selection.selected_logic_items().size() != 1 ||
        !selection.selected_segments().empty()) {
        return null_element;
    }
    return selection.selected_logic_items().front();
}

}  // namespace

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto element_id = get_single_logic_item(selection);
    if (!element_id) {
        return {};
    }
    if (layout.display_state(element_id) != display_state_t::normal) {
        return {};
    }

    return setting_handle_position(layout, element_id);
}

auto setting_handle_rect(setting_handle_t handle, const ViewConfig& config)
    -> rect_fine_t {
    return to_rect(handle.position, defaults::setting_handle_size);
}

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection, const ViewConfig& config)
    -> std::optional<setting_handle_t> {
    const auto handle = setting_handle_position(layout, selection);

    if (!handle) {
        return {};
    }

    const auto rect = setting_handle_rect(*handle, config);
    if (is_colliding(position, rect)) {
        return handle;
    }

    return {};
}

//
}  // namespace logicsim