#include "core/setting_handle.h"

#include "core/geometry/layout_calculation.h"
#include "core/geometry/rect.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/selection.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/rect_fine.h"

#include <exception>

namespace logicsim {
auto setting_handle_position(const Layout& layout, logicitem_id_t logicitem_id)
    -> std::optional<setting_handle_t> {
    switch (layout.logicitems().type(logicitem_id)) {
        using enum LogicItemType;

        case clock_generator: {
            // TODO move to logicitem/layout.h

            // constexpr auto overdraw = defaults::logicitem_body_overdraw;
            constexpr auto handle_size = defaults::setting_handle_size;
            // constexpr auto margin = defaults::setting_handle_margin;

            const auto width = element_fixed_width(clock_generator);
            const auto height = element_fixed_height(clock_generator);

            const auto position = layout.logicitems().position(logicitem_id);
            const auto orientation = layout.logicitems().orientation(logicitem_id);

            return setting_handle_t {
                .position =
                    transform(position, orientation,
                              point_fine_t {
                                  // width - handle_size / 2.0 - margin,
                                  // margin + handle_size / 2.0,
                                  // height + overdraw - handle_size / 2.0 - margin,
                                  width / 2.0,
                                  height / 2.0 + handle_size / 2.0,
                              }),
                .icon = icon_t::setting_handle_clock_generator,
                .logicitem_id = logicitem_id,
            };
        }

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

    std::terminate();
}

namespace {

auto get_single_logicitem(const Selection& selection) -> logicitem_id_t {
    if (selection.selected_logicitems().size() != 1 ||
        !selection.selected_segments().empty()) {
        return null_logicitem_id;
    }
    return selection.selected_logicitems().front();
}

}  // namespace

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto logicitem_id = get_single_logicitem(selection);
    if (!logicitem_id) {
        return {};
    }
    if (layout.logicitems().display_state(logicitem_id) != display_state_t::normal) {
        return {};
    }

    return setting_handle_position(layout, logicitem_id);
}

auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t {
    return to_rect(handle.position, defaults::setting_handle_size);
}

auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool {
    return is_colliding(position, setting_handle_rect(handle));
}

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto handle = setting_handle_position(layout, selection);

    if (handle && is_colliding(*handle, position)) {
        return handle;
    }

    return {};
}

}  // namespace logicsim
