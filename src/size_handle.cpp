#include "size_handle.h"

#include "algorithm/round.h"
#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection.h"
#include "exception.h"
#include "geometry/point.h"
#include "geometry/rect.h"
#include "geometry/scene.h"
#include "layout.h"
#include "layout_calculation.h"
#include "safe_numeric.h"
#include "vocabulary/view_config.h"

#include <blend2d.h>

namespace logicsim {

auto size_handle_positions(const layout::ConstElement element)
    -> std::vector<size_handle_t> {
    switch (element.element_type()) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element: {
            const auto height = standard_element::height(element.input_count());
            constexpr auto overdraw = defaults::logic_item_body_overdraw;

            return {
                size_handle_t {0, transform(element.position(), element.orientation(),
                                            point_fine_t {1.0, -overdraw})},
                size_handle_t {1, transform(element.position(), element.orientation(),
                                            point_fine_t {1.0, height + overdraw})},
            };
        }

        case display_number: {
            const auto value_inputs = display_number::value_inputs(element.input_count());

            const auto width = display_number::width(element.input_count());
            const auto height = display_number::height(element.input_count());

            constexpr auto overdraw = defaults::logic_item_body_overdraw;

            return {
                size_handle_t {1, transform(element.position(), element.orientation(),
                                            point_fine_t {
                                                0.5 * width,
                                                height + overdraw,
                                            })},
            };
        }

        case unused:
        case placeholder:
        case wire:

        case buffer_element:
        case button:
        case led:
        case display_ascii:

        case clock_generator:
        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:

        case sub_circuit:
            return {};
    };

    throw_exception("unknown ElementType in size_handle_positions");
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

auto size_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<size_handle_t> {
    // only show handles when a single item is selected
    const auto element_id = get_single_logic_item(selection);
    if (!element_id) {
        return {};
    }
    if (layout.display_state(element_id) == display_state_t::colliding) {
        return {};
    }

    return size_handle_positions(layout.element(element_id));
}

auto size_handle_rect_px(size_handle_t handle, const ViewConfig& config) -> BLRect {
    const auto rect_size_dev = defaults::size_handle_rect_size_device;

    const auto [x, y] = to_context(handle.point, config);
    const auto size = rect_size_dev * config.device_pixel_ratio();

    const auto x0 = round_fast(x - size / 2);
    const auto y0 = round_fast(y - size / 2);
    const auto s = round_fast(size);

    return BLRect(x0, y0, s, s);
}

auto size_handle_rect_grid(size_handle_t handle, const ViewConfig& config)
    -> rect_fine_t {
    const auto rect = size_handle_rect_px(handle, config);
    return rect_fine_t {
        to_grid_fine(BLPoint {rect.x, rect.y}, config),
        to_grid_fine(BLPoint {rect.x + rect.w, rect.y + rect.h}, config),
    };
}

auto is_size_handle_colliding(point_fine_t position, size_handle_t handle_positions,
                              const ViewConfig& config) -> bool {
    const auto rect = size_handle_rect_grid(handle_positions, config);
    return is_colliding(position, rect);
}

auto get_colliding_size_handle(point_fine_t position,
                               const std::vector<size_handle_t>& handle_positions,
                               const ViewConfig& config) -> std::optional<size_handle_t> {
    const auto is_colliding = [&](size_handle_t handle) -> bool {
        return is_size_handle_colliding(position, handle, config);
    };

    if (const auto it = std::ranges::find_if(handle_positions, is_colliding);
        it != handle_positions.end()) {
        return *it;
    }
    return {};
}

auto get_colliding_size_handle(point_fine_t position, const Layout& layout,
                               const Selection& selection, const ViewConfig& config)
    -> std::optional<size_handle_t> {
    const auto handles = size_handle_positions(layout, selection);
    return get_colliding_size_handle(position, handles, config);
}

//
// Change Logic
//

namespace size_handle {

auto clamp_connection_count(connection_count_t count, int delta, connection_count_t min,
                            connection_count_t max) -> connection_count_t {
    const auto new_count = count.safe_value() + ls_safe<int> {delta};

    const auto clamped_count =
        std::clamp<decltype(new_count)>(new_count, min.safe_value(), max.safe_value());

    return connection_count_t {clamped_count};
}

auto adjust_height(const PlacedElement& original, size_handle_t handle, int delta,
                   connection_count_t min_inputs, connection_count_t max_inputs,
                   std::invocable<connection_count_t> auto get_height) {
    if (handle.index != 0 && handle.index != 1) {
        throw_exception("unknown handle index");
    }

    auto result = PlacedElement {original};

    // input count
    if (handle.index == 0) {
        result.definition.input_count = clamp_connection_count(
            original.definition.input_count, -delta, min_inputs, max_inputs);
    } else if (handle.index == 1) {
        result.definition.input_count = clamp_connection_count(
            original.definition.input_count, +delta, min_inputs, max_inputs);
    }

    // position adjustment
    if (handle.index == 0) {
        const auto old_height = get_height(original.definition.input_count);
        const auto new_height = get_height(result.definition.input_count);
        const auto delta_height = int {old_height} - int {new_height};

        if (is_representable(original.position, 0, delta_height)) {
            result.position = add_unchecked(original.position, 0, delta_height);
        } else {
            return original;
        }
    }

    // inverters
    result.definition.input_inverters.resize(result.definition.input_count.count());
    return result;
}

auto transform_item(const PlacedElement& original, size_handle_t handle, int delta)
    -> PlacedElement {
    switch (original.definition.element_type) {
        using enum ElementType;

        case and_element:
        case or_element:
        case xor_element: {
            return adjust_height(original, handle, delta, standard_element::min_inputs,
                                 standard_element::max_inputs, standard_element::height);
        }
        case display_number: {
            return adjust_height(original, handle, delta, display_number::min_inputs,
                                 display_number::max_inputs, display_number::height);
        }

        case unused:
        case placeholder:
        case wire:

        case buffer_element:
        case button:
        case led:
        case display_ascii:

        case clock_generator:
        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:

        case sub_circuit:
            throw_exception("not supported");
    };

    throw_exception("unknown ElementType in size_handle::transform_item");
}

auto get_logic_item(const EditableCircuit& editable_circuit) -> PlacedElement {
    const auto& selection = editable_circuit.selection_builder().selection();
    const auto& element_id = get_single_logic_item(selection);

    return to_placed_element(editable_circuit.layout(), element_id);
}

}  // namespace size_handle

//
// Mouse Size Handle Logic
//

MouseSizeHandleLogic::MouseSizeHandleLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit},
      size_handle_ {args.size_handle},
      initial_logic_item_ {size_handle::get_logic_item(editable_circuit_)} {}

MouseSizeHandleLogic::~MouseSizeHandleLogic() {
    if (temp_item_) {
        move_handle(*first_position_);
    }
    if (temp_item_colliding()) [[unlikely]] {
        throw_exception("unexpected collision");
    }
}

auto MouseSizeHandleLogic::mouse_press(point_fine_t position) -> void {
    first_position_ = position;
    last_delta_ = 0;
}

auto MouseSizeHandleLogic::mouse_move(point_fine_t position) -> void {
    move_handle(position);
}

auto MouseSizeHandleLogic::mouse_release(point_fine_t position) -> void {
    move_handle(position);

    // mark as permanent
    if (temp_item_exists() && !temp_item_colliding()) {
        temp_item_.reset();
    }
}

auto MouseSizeHandleLogic::move_handle(point_fine_t position) -> void {
    if (!first_position_ || !last_delta_) {
        return;
    }

    const auto new_delta = round_to<int>(double {position.y - first_position_->y});
    if (new_delta == *last_delta_) {
        return;
    }
    last_delta_ = new_delta;

    // delete element
    auto& builder = editable_circuit_.selection_builder();
    editable_circuit_.delete_all(editable_circuit_.get_handle(builder.selection()));

    // add transformed
    const auto logic_item =
        size_handle::transform_item(initial_logic_item_, size_handle_, new_delta);
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

auto MouseSizeHandleLogic::temp_item_colliding() const -> bool {
    return temp_item_ &&
           anything_colliding(temp_item_.value(), editable_circuit_.layout());
}

auto MouseSizeHandleLogic::temp_item_exists() const -> bool {
    return temp_item_ && !temp_item_->selected_logic_items().empty();
}

}  // namespace logicsim
