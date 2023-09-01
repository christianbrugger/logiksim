#include "drag_handle.h"

#include "collision.h"
#include "editable_circuit/editable_circuit.h"
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
                drag_handle_t {0, transform(element.position(), element.orientation(),
                                            point_fine_t {1.0, -overdraw})},
                drag_handle_t {1, transform(element.position(), element.orientation(),
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

auto get_single_logic_item(const Selection& selection) -> element_id_t {
    if (selection.selected_logic_items().size() != 1 ||
        !selection.selected_segments().empty()) {
        return null_element;
    }
    return selection.selected_logic_items().front();
}

auto drag_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<drag_handle_t> {
    // only show handles when a single item is selected
    const auto element_id = get_single_logic_item(selection);
    if (!element_id) {
        return {};
    }
    return drag_handle_positions(layout.element(element_id));
}

auto drag_handle_rect_px(drag_handle_t handle_position, const ViewConfig& config)
    -> BLRect {
    const auto rect_size_lp = defaults::drag_handle_rect_size_device;

    const auto [x, y] = to_context(handle_position.point, config);
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

auto get_colliding_handle(point_fine_t position,
                          const std::vector<drag_handle_t>& handle_positions,
                          const ViewConfig& config) -> std::optional<drag_handle_t> {
    const auto is_colliding = [&](drag_handle_t handle) -> bool {
        return is_drag_handle_colliding(position, handle, config);
    };

    if (const auto it = std::ranges::find_if(handle_positions, is_colliding);
        it != handle_positions.end()) {
        return *it;
    }
    return {};
}

auto get_colliding_handle(point_fine_t position, const Layout& layout,
                          const Selection& selection, const ViewConfig& config)
    -> std::optional<drag_handle_t> {
    const auto handles = drag_handle_positions(layout, selection);
    return get_colliding_handle(position, handles, config);
}

//
// Change Logic
//

namespace drag_handle {

auto transform_item(const logic_item_t original, drag_handle_t handle, int delta)
    -> logic_item_t {
    switch (original.definition.element_type) {
        using enum ElementType;

        case unused:
        case placeholder:
        case wire:
            throw_exception("not supported");

        case buffer_element:
            throw_exception("not supported");

        case and_element:
        case or_element:
        case xor_element: {
            const auto input_count = gsl::narrow<int>(original.definition.input_count);
            auto result = logic_item_t {original};
            // TODO !!! put 2 and 127 in global constant

            if (handle.index == 0) {
                const auto delta_clamped =
                    input_count - std::clamp(input_count - delta, 2, 127);

                if (is_representable(original.position, 0, delta_clamped)) {
                    result.position = add_unchecked(original.position, 0, delta_clamped);
                    result.definition.input_count =
                        std::clamp(input_count - delta_clamped, 2, 127);
                }
            } else if (handle.index == 1) {
                result.definition.input_count = std::clamp(input_count + delta, 2, 127);
            } else {
                throw_exception("unknown handle index");
            }

            result.definition.input_inverters.resize(result.definition.input_count);
            return result;
        }

        case button:
        case led:
        case display_number:
        case display_ascii:
            throw_exception("not supported");

        case clock_generator:
        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:
            throw_exception("not supported");

        case sub_circuit:
            throw_exception("not supported");
    };

    throw_exception("unknown ElementType in drag_handle::transform_item");
}

auto get_logic_item(const EditableCircuit& editable_circuit) -> logic_item_t {
    const auto& selection = editable_circuit.selection_builder().selection();
    const auto& element_id = get_single_logic_item(selection);

    return logic_item_t {
        .definition = editable_circuit.get_logic_item_definition(element_id),
        .position = editable_circuit.layout().position(element_id),
    };
}

}  // namespace drag_handle

//
// Mouse Drag Handle Logic
//

MouseDragHandleLogic::MouseDragHandleLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit},
      drag_handle_ {args.drag_handle},
      initial_logic_item_ {drag_handle::get_logic_item(editable_circuit_)} {}

MouseDragHandleLogic::~MouseDragHandleLogic() {
    if (temp_item_) {
        move_handle(*first_position_);
    }
    if (temp_item_colliding()) [[unlikely]] {
        throw_exception("unexpected collision");
    }
}

auto MouseDragHandleLogic::mouse_press(point_fine_t position) -> void {
    first_position_ = position;
    last_delta_ = 0;
}

auto MouseDragHandleLogic::mouse_move(point_fine_t position) -> void {
    move_handle(position);
}

auto MouseDragHandleLogic::mouse_release(point_fine_t position) -> void {
    move_handle(position);

    // mark as permanent
    if (!temp_item_colliding()) {
        temp_item_.reset();
    }
}

auto MouseDragHandleLogic::move_handle(point_fine_t position) -> void {
    if (!first_position_ || !last_delta_) {
        return;
    }

    const auto new_delta = gsl::narrow<int>(round_fast(position.y - first_position_->y));
    if (new_delta == *last_delta_) {
        return;
    }
    last_delta_ = new_delta;

    // delete element
    auto& builder = editable_circuit_.selection_builder();
    editable_circuit_.delete_all(editable_circuit_.get_handle(builder.selection()));

    // add transformed
    const auto logic_item =
        drag_handle::transform_item(initial_logic_item_, drag_handle_, new_delta);
    temp_item_ = editable_circuit_.add_logic_item(
        logic_item.definition, logic_item.position, InsertionMode::collisions);

    // mark selected
    builder.set_selection(temp_item_.value());

    // check collisions
    if (!temp_item_colliding()) {
        editable_circuit_.change_insertion_mode(temp_item_.copy(),
                                                InsertionMode::insert_or_discard);
    }
}

auto MouseDragHandleLogic::temp_item_colliding() -> bool {
    return temp_item_ &&
           anything_colliding(temp_item_.value(), editable_circuit_.layout());
}

}  // namespace logicsim
