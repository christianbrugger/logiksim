#include "drag_handle.h"

#include "collision.h"
#include "editable_circuit/selection.h"
#include "exception.h"
#include "layout.h"
#include "layout_calculation.h"
#include "scene.h"

namespace logicsim {

auto drag_handle_positions(const layout::ConstElement element)
    -> std::vector<drag_handle_t> {
    switch (element.element_type()) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:
            return {};

        case buffer_element:
            return {};
        case and_element:
        case or_element:
        case xor_element: {
            require_min(element.input_count(), 2);

            const auto height = element.input_count() - 1.0;
            constexpr auto overdraw = defaults::logic_item_body_overdraw;

            return {
                drag_handle_t {transform(element.position(), element.orientation(),
                                         point_fine_t {1.0, -overdraw})},
                drag_handle_t {transform(element.position(), element.orientation(),
                                         point_fine_t {1.0, height + overdraw})},
            };
        }

        case button:
        case led:
        case display_number:
        case display_ascii:
            return {};

        case clock_generator:
        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:
            return {};

        case sub_circuit:
            return {};
    };

    throw_exception("unknown ElementType in drag_handle_positions");
}

auto drag_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<drag_handle_t> {
    // only when single item is selected
    if (selection.selected_logic_items().size() != 1 ||
        !selection.selected_segments().empty()) {
        return {};
    }
    const auto element = layout.element(selection.selected_logic_items().front());

    // only inserted elements have handles
    // if (element.display_state() != display_state_t::normal) {
    //    return {};
    //}

    return drag_handle_positions(element);
}

auto drag_handle_rect_px(drag_handle_t handle_position, const ViewConfig& config)
    -> BLRect {
    const auto rect_size_lp = defaults::drag_handle_rect_size_device;

    const auto [x, y] = to_context(handle_position.value, config);
    const auto width = rect_size_lp * config.device_pixel_ratio();

    const auto x0 = round_fast(x - width / 2);
    const auto y0 = round_fast(y - width / 2);
    const auto w = round_fast(width);

    return BLRect(x0, y0, w, w);
}

auto drag_handle_rect_gird(drag_handle_t handle_position, const ViewConfig& config)
    -> rect_fine_t {
    const auto rect = drag_handle_rect_px(handle_position, config);
    return rect_fine_t {
        from_context_fine(BLPoint {rect.x, rect.y}, config),
        from_context_fine(BLPoint {rect.x + rect.w, rect.y + rect.h}, config),
    };
}

auto is_drag_handle_colliding(point_fine_t position, drag_handle_t handle_positions,
                              const ViewConfig& config) -> bool {
    const auto rect = drag_handle_rect_gird(handle_positions, config);
    return is_colliding(position, rect);
}

auto is_drag_handle_colliding(point_fine_t position,
                              const std::vector<drag_handle_t>& handle_positions,
                              const ViewConfig& config) -> bool {
    return std::ranges::any_of(handle_positions, [&](drag_handle_t handle) -> bool {
        return is_drag_handle_colliding(position, handle, config);
    });
}

auto is_drag_handle_colliding(point_fine_t position, const Layout& layout,
                              const Selection& selection, const ViewConfig& config)
    -> bool {
    const auto handles = drag_handle_positions(layout, selection);
    return is_drag_handle_colliding(position, handles, config);
}

//
// Mouse Drag Handle Logic
//

MouseDragHandleLogic::MouseDragHandleLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit} {}

auto MouseDragHandleLogic::mouse_press(point_fine_t position) -> void {}

auto MouseDragHandleLogic::mouse_move(point_fine_t position) -> void {}

auto MouseDragHandleLogic::mouse_release(point_fine_t position) -> void {}

}  // namespace logicsim
