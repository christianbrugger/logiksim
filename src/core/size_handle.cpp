#include "core/size_handle.h"

#include "core/algorithm/round.h"
#include "core/editable_circuit.h"
#include "core/element/logicitem/layout_logicitem_display_number.h"
#include "core/geometry/connection_count.h"
#include "core/geometry/layout_calculation.h"
#include "core/geometry/offset.h"
#include "core/geometry/point.h"
#include "core/geometry/rect.h"
#include "core/geometry/scene.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/safe_numeric.h"
#include "core/selection.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/placed_decoration.h"
#include "core/vocabulary/placed_logicitem.h"
#include "core/vocabulary/view_config.h"

#include <blend2d.h>

#include <stdexcept>

namespace logicsim {
auto size_handle_t::format() const -> std::string {
    return fmt::format("size_handle_t(index = {}, point = {})", index, point);
}

auto delta_movement_t::format() const -> std::string {
    return fmt::format("delta_movement_t(horizontal = {}, vertical = {})", horizontal,
                       vertical);
}

auto size_handle_positions(const Layout& layout,
                           logicitem_id_t logicitem_id) -> std::vector<size_handle_t> {
    switch (layout.logicitems().type(logicitem_id)) {
        using enum LogicItemType;

        case and_element:
        case or_element:
        case xor_element: {
            // TODO move to logicitem/layout.h
            const auto overdraw = logicitem_body_overdraw();
            const auto data = to_layout_calculation_data(layout, logicitem_id);
            const auto width = element_width(data);
            const auto height = element_height(data);

            const auto position = layout.logicitems().position(logicitem_id);
            const auto orientation = layout.logicitems().orientation(logicitem_id);

            return {
                size_handle_t {0, transform(position, orientation,
                                            point_fine_t {width / 2., -overdraw})},
                size_handle_t {1,
                               transform(position, orientation,
                                         point_fine_t {width / 2., height + overdraw})},
            };
        }

        case display_number: {
            // TODO move to logicitem/layout.h
            const auto overdraw = logicitem_body_overdraw();
            const auto input_count = layout.logicitems().input_count(logicitem_id);
            const auto width = display_number::width(input_count);

            static_assert(display_number::min_value_inputs >= connection_count_t {1});
            const auto last_input_y = to_grid(display_number::value_inputs(input_count) -
                                              connection_count_t {1});

            const auto position = layout.logicitems().position(logicitem_id);
            const auto orientation = layout.logicitems().orientation(logicitem_id);

            return {
                size_handle_t {1, transform(position, orientation,
                                            point_fine_t {
                                                0.5 * width,
                                                last_input_y + overdraw,
                                            })},
            };
        }

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

    std::terminate();
}

auto size_handle_positions(const Layout& layout,
                           decoration_id_t decoration_id) -> std::vector<size_handle_t> {
    switch (layout.decorations().type(decoration_id)) {
        using enum DecorationType;

        case text_element: {
            // TODO move to element/decoration/.h
            const auto position = layout.decorations().position(decoration_id);
            const auto size = layout.decorations().size(decoration_id);

            const auto position_end = point_t {
                to_grid(size.width, position.x),
                to_grid(size.height, position.y),
            };
            const auto offset = point_fine_t {0.5, 0.5};

            return {
                size_handle_t {0, point_fine_t {position} - offset},
                size_handle_t {1, point_fine_t {position_end} + offset},
            };
        }
    };

    std::terminate();
}

auto size_handle_positions(const Layout& layout,
                           const Selection& selection) -> std::vector<size_handle_t> {
    if (const auto logicitem_id = get_single_logicitem(selection);
        logicitem_id &&
        layout.logicitems().display_state(logicitem_id) == display_state_t::normal) {
        return size_handle_positions(layout, logicitem_id);
    }

    if (const auto decoration_id = get_single_decoration(selection);
        decoration_id &&
        layout.decorations().display_state(decoration_id) == display_state_t::normal) {
        return size_handle_positions(layout, decoration_id);
    }

    return {};
}

auto size_handle_rect_px(size_handle_t handle, const ViewConfig& config) -> BLRect {
    const auto rect_size_dev = defaults::size_handle_rect_size_device;

    const auto [x, y] = to_context(handle.point, config);
    const auto size = rect_size_dev * config.device_pixel_ratio();

    const auto x0 = round_fast(x - size / 2);
    const auto y0 = round_fast(y - size / 2);
    const auto s = round_fast(size);

    return BLRect {x0, y0, s, s};
}

auto size_handle_rect_grid(size_handle_t handle,
                           const ViewConfig& config) -> rect_fine_t {
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
                               const Selection& selection,
                               const ViewConfig& config) -> std::optional<size_handle_t> {
    const auto handles = size_handle_positions(layout, selection);
    return get_colliding_size_handle(position, handles, config);
}

//
// Change Logic
//

namespace {

auto clamp_connection_count(connection_count_t count, int delta, connection_count_t min,
                            connection_count_t max) -> connection_count_t {
    const auto new_count = count.safe_value() + ls_safe<int> {delta};

    const auto clamped_count =
        std::clamp<decltype(new_count)>(new_count, min.safe_value(), max.safe_value());

    return connection_count_t {clamped_count};
}

auto logicitem_height(const PlacedLogicItem& element) -> grid_t {
    const auto data = to_layout_calculation_data(element);
    return element_height(data);
}

[[nodiscard]] auto adjust_logicitem_height(const PlacedLogicItem& original,
                                           size_handle_t handle,
                                           delta_movement_t delta) -> PlacedLogicItem {
    if (handle.index != 0 && handle.index != 1) {
        throw std::runtime_error("unknown handle index");
    }

    auto result = PlacedLogicItem {original};

    const auto min_inputs = element_input_count_min(original.definition.logicitem_type);
    const auto max_inputs = element_input_count_max(original.definition.logicitem_type);

    // input count
    if (handle.index == 0) {
        result.definition.input_count = clamp_connection_count(
            original.definition.input_count, -delta.vertical, min_inputs, max_inputs);
    } else if (handle.index == 1) {
        result.definition.input_count = clamp_connection_count(
            original.definition.input_count, +delta.vertical, min_inputs, max_inputs);
    }

    // position adjustment
    if (handle.index == 0) {
        const auto old_height = logicitem_height(original);
        const auto new_height = logicitem_height(result);
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

}  // namespace

auto get_resized_element(const PlacedLogicItem& original, size_handle_t handle,
                         delta_movement_t delta) -> PlacedLogicItem {
    switch (original.definition.logicitem_type) {
        using enum LogicItemType;

        case and_element:
        case or_element:
        case xor_element:

        case display_number: {
            return adjust_logicitem_height(original, handle, delta);
        }

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
            throw std::runtime_error("element does not supported resizing");
    };

    std::terminate();
}

namespace {

auto clamp_offset(offset_t width, int delta, offset_t min, offset_t max) -> offset_t {
    const auto new_offset = int64_t {int {width}} + int64_t {delta};
    static_assert(sizeof(new_offset) > sizeof(delta));

    const auto clamped_count =
        std::clamp<decltype(new_offset)>(new_offset, int {min}, int {max});

    return offset_t {clamped_count};
}

[[nodiscard]] auto adjust_decoration_size(const PlacedDecoration& original,
                                          size_handle_t handle, delta_movement_t delta) {
    if (handle.index != 0 && handle.index != 1) {
        throw std::runtime_error("unknown handle index");
    }

    auto result = PlacedDecoration {original};
    const auto min_size = element_size_min(original.definition.decoration_type);
    const auto max_size = element_size_max(original.definition.decoration_type);

    // width
    const auto delta_width = handle.index == 0 ? -delta.horizontal : delta.horizontal;
    result.definition.size.width = clamp_offset(
        original.definition.size.width, delta_width, min_size.width, max_size.width);

    // height
    const auto delta_height = handle.index == 0 ? -delta.vertical : delta.vertical;
    result.definition.size.height = clamp_offset(
        original.definition.size.height, delta_height, min_size.height, max_size.height);

    // position adjustment
    const auto old_size = original.definition.size;
    const auto new_size = result.definition.size;
    const auto width_diff = int {new_size.width} - int {old_size.width};
    const auto height_diff = int {new_size.height} - int {old_size.height};

    const auto delta_x = handle.index == 0 ? -width_diff : 0;
    const auto delta_y = handle.index == 0 ? -height_diff : 0;

    if (is_representable(original.position, delta_x, delta_y)) {
        result.position = add_unchecked(original.position, delta_x, delta_y);
    } else {
        return original;
    }

    return result;
}

}  // namespace

auto get_resized_element(const PlacedDecoration& original, size_handle_t handle,
                         delta_movement_t delta) -> PlacedDecoration {
    switch (original.definition.decoration_type) {
        using enum DecorationType;

        case text_element: {
            return adjust_decoration_size(original, handle, delta);
        }
    }

    std::terminate();
}

auto get_resized_element(const PlacedElement& original, size_handle_t handle,
                         delta_movement_t delta) -> PlacedElement {
    return std::visit(
        [&](const auto& element) -> PlacedElement {
            return get_resized_element(element, handle, delta);
        },
        original);
}

auto get_single_placed_element(const EditableCircuit& editable_circuit)
    -> std::optional<PlacedElement> {
    if (const auto logicitem_id =
            get_single_logicitem(editable_circuit.visible_selection());
        logicitem_id) {
        return to_placed_logicitem(editable_circuit.layout(), logicitem_id);
    }
    if (const auto decoration_id =
            get_single_decoration(editable_circuit.visible_selection());
        decoration_id) {
        return to_placed_decoration(editable_circuit.layout(), decoration_id);
    }

    return std::nullopt;
}

}  // namespace logicsim
