#include "size_handle.h"

#include "algorithm/round.h"
#include "editable_circuit.h"
#include "element/logicitem/layout_display_number.h"
#include "geometry/connection_count.h"
#include "geometry/layout_calculation.h"
#include "geometry/point.h"
#include "geometry/rect.h"
#include "geometry/scene.h"
#include "layout.h"
#include "layout_info.h"
#include "safe_numeric.h"
#include "selection.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/placed_element.h"
#include "vocabulary/view_config.h"

#include <blend2d.h>

#include <stdexcept>

namespace logicsim {

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

    throw std::runtime_error("unknown ElementType in size_handle_positions");
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

auto size_handle_positions(const Layout& layout,
                           const Selection& selection) -> std::vector<size_handle_t> {
    // only show handles when a single item is selected
    const auto logicitem_id = get_single_logicitem(selection);
    if (!logicitem_id) {
        return {};
    }
    if (layout.logicitems().display_state(logicitem_id) != display_state_t::normal) {
        return {};
    }

    return size_handle_positions(layout, logicitem_id);
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

auto element_height(const PlacedElement& element) -> grid_t {
    const auto data = to_layout_calculation_data(element);
    return element_height(data);
}

auto adjust_height(const PlacedElement& original, size_handle_t handle, int delta) {
    if (handle.index != 0 && handle.index != 1) {
        throw std::runtime_error("unknown handle index");
    }

    auto result = PlacedElement {original};

    const auto min_inputs = element_input_count_min(original.definition.logicitem_type);
    const auto max_inputs = element_input_count_max(original.definition.logicitem_type);

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
        const auto old_height = element_height(original);
        const auto new_height = element_height(result);
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

auto get_resized_element(const PlacedElement& original, size_handle_t handle,
                         int delta) -> PlacedElement {
    switch (original.definition.logicitem_type) {
        using enum LogicItemType;

        case and_element:
        case or_element:
        case xor_element:

        case display_number: {
            return adjust_height(original, handle, delta);
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

auto get_single_placed_element(const EditableCircuit& editable_circuit) -> PlacedElement {
    const auto element_id = get_single_logicitem(editable_circuit.visible_selection());
    Expects(element_id);

    return to_placed_element(editable_circuit.layout(), element_id);
}

}  // namespace logicsim
