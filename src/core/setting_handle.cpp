#include "core/setting_handle.h"

#include "core/algorithm/overload.h"
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

auto setting_handle_t::format() const -> std::string {
    const auto id_str =
        std::visit(overload(
                       [](const logicitem_id_t logicitem_id) {
                           return fmt::format("logicitem_id = {}", logicitem_id);
                       },

                       [](const decoration_id_t decoration_id) {
                           return fmt::format("decoration_id = {}", decoration_id);
                       }),
                   element_id);
    return fmt::format("setting_handle_t(position = {}, icon = {}, {})", position, icon,
                       id_str);
}

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
                .element_id = logicitem_id,
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

auto setting_handle_position(const Layout& layout, decoration_id_t decoration_id)
    -> std::optional<setting_handle_t> {
    switch (layout.decorations().type(decoration_id)) {
        using enum DecorationType;

        case text_element: {
            // TODO move to element/decoration/..

            const auto position = layout.decorations().position(decoration_id);
            const auto size = layout.decorations().size(decoration_id);

            return setting_handle_t {
                .position =
                    point_fine_t {position} +
                    point_fine_t {int {size.width} / 2.0, int {size.height} / 2.0},
                .icon = icon_t::setting_handle_clock_generator,
                .element_id = decoration_id,
            };
        }
    };

    std::terminate();
}

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t> {
    if (const auto logicitem_id = get_single_logicitem(selection);
        logicitem_id &&
        layout.logicitems().display_state(logicitem_id) == display_state_t::normal) {
        return setting_handle_position(layout, logicitem_id);
    }
    if (const auto decoration_id = get_single_decoration(selection);
        decoration_id &&
        layout.decorations().display_state(decoration_id) == display_state_t::normal) {
        return setting_handle_position(layout, decoration_id);
    }

    return std::nullopt;
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
