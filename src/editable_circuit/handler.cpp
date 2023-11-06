#include "editable_circuit/handler.h"

#include "algorithm/range.h"
#include "algorithm/sort_pair.h"
#include "algorithm/transform_to_vector.h"
#include "editable_circuit/cache.h"
#include "editable_circuit/cache/split_point_cache.h"
#include "editable_circuit/selection.h"
#include "editable_circuit/selection_registrar.h"
#include "exception.h"
#include "format/container.h"
#include "format/pointer.h"
#include "geometry/line.h"
#include "geometry/orientation.h"
#include "geometry/point.h"
#include "layout_info.h"
#include "logging.h"
#include "tree_normalization.h"

#include <ankerl/unordered_dense.h>
#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <ranges>

namespace logicsim {

namespace editable_circuit {

//
// Wire Input Conversions
//

namespace {

struct wire_connection_t {
    point_t position;
    segment_t segment;

    auto format() const -> std::string {
        return fmt::format("({}, {})", position, segment);
    }
};

using policy = folly::small_vector_policy::policy_size_type<uint32_t>;
using wire_connections_t = folly::small_vector<wire_connection_t, 3, policy>;

static_assert(sizeof(wire_connection_t) == 12);
static_assert(sizeof(wire_connections_t) == 40);

auto has_duplicate_wire_ids(wire_connections_t connections) -> bool {
    auto to_wire_id = [](wire_connection_t input) { return input.segment.wire_id; };

    std::ranges::sort(connections, std::ranges::less {}, to_wire_id);

    return std::ranges::adjacent_find(connections, std::ranges::equal_to {},
                                      to_wire_id) != connections.end();
}

auto is_convertible_to_input(const Layout& layout, wire_id_t wire_id) -> bool {
    return !layout.wires().segment_tree(wire_id).has_input();
}

auto all_convertible_to_input(const Layout& layout, wire_connections_t connections)
    -> bool {
    return std::ranges::all_of(connections, [&](wire_connection_t input) {
        return is_convertible_to_input(layout, input.segment.wire_id);
    });
}

struct convertible_inputs_result_t {
    wire_connections_t convertible_inputs {};
    bool any_collisions {false};

    auto format() const -> std::string {
        return fmt::format("<any_collisions = {}, convertible_inputs = {}>",
                           any_collisions, convertible_inputs);
    }
};

auto find_convertible_wire_input_candiates(const CacheProvider& cache,
                                           const layout_calculation_data_t& data)
    -> convertible_inputs_result_t {
    auto result = convertible_inputs_result_t {};

    for (const auto& info : output_locations(data)) {
        if (const auto entry = cache.wire_output_cache().find(info.position)) {
            // not compatible
            if (!orientations_compatible(info.orientation, entry->orientation)) {
                return {.any_collisions = true};
            }

            result.convertible_inputs.push_back({info.position, entry->segment});
        }
    }

    return result;
}

auto find_convertible_wire_inputs(const Layout& layout, const CacheProvider& cache,
                                  const layout_calculation_data_t& data)
    -> convertible_inputs_result_t {
    auto candidates = find_convertible_wire_input_candiates(cache, data);

    if (candidates.any_collisions ||
        has_duplicate_wire_ids(candidates.convertible_inputs) ||
        !all_convertible_to_input(layout, candidates.convertible_inputs)) {
        return {.any_collisions = true};
    }

    return candidates;
}

auto assert_equal_type(SegmentPointType type, SegmentPointType expected) -> void {
    if (type != expected) [[unlikely]] {
        throw_exception("type is not of expected type");
    }
}

auto convert_from_to(Layout& layout, MessageSender& sender, wire_connection_t output,
                     SegmentPointType from_type, SegmentPointType to_type) {
    if (!is_inserted(output.segment.wire_id)) [[unlikely]] {
        throw_exception("can only convert inserted wires");
    }

    auto& m_tree = layout.wires().modifyable_segment_tree(output.segment.wire_id);
    const auto old_info = m_tree.info(output.segment.segment_index);
    auto new_info = old_info;

    if (new_info.line.p0 == output.position) {
        assert_equal_type(new_info.p0_type, from_type);
        new_info.p0_type = to_type;
    }

    else if (new_info.line.p1 == output.position) {
        assert_equal_type(new_info.p1_type, from_type);
        new_info.p1_type = to_type;
    }

    else [[unlikely]] {
        throw_exception("connector position is not part of segment line");
    }

    m_tree.update_segment(output.segment.segment_index, new_info);

    sender.submit(info_message::InsertedEndPointsUpdated {
        .segment = output.segment,
        .new_segment_info = new_info,
        .old_segment_info = old_info,
    });
}

auto convert_to_input(Layout& layout, MessageSender& sender, wire_connection_t output) {
    convert_from_to(layout, sender, output, SegmentPointType::output,
                    SegmentPointType::input);
}

auto convert_to_output(Layout& layout, MessageSender& sender, wire_connection_t output) {
    convert_from_to(layout, sender, output, SegmentPointType::input,
                    SegmentPointType::output);
}

auto convert_to_inputs(Layout& layout, MessageSender& sender,
                       wire_connections_t outputs) {
    for (auto output : outputs) {
        convert_to_input(layout, sender, output);
    }
}

}  // namespace

//
// Deletion Handling
//

/*
auto notify_element_deleted(const Layout& layout, MessageSender& sender,
                            element_id_t element_id) {
    const auto element = layout.element(element_id);

    if (element.is_logic_item()) {
        sender.submit(info_message::LogicItemDeleted {element_id});
    }
}

auto notify_element_id_change(const Layout& layout, MessageSender& sender,
                              const element_id_t new_element_id,
                              const element_id_t old_element_id) {
    const auto element = layout.element(new_element_id);

    if (element.is_placeholder()) {
        return;
    }

    const bool inserted = is_inserted(layout, new_element_id);

    if (element.is_logic_item()) {
        sender.submit(info_message::LogicItemIdUpdated {
            .new_element_id = new_element_id,
            .old_element_id = old_element_id,
        });
    }

    if (element.is_logic_item() && inserted) {
        const auto data = to_layout_calculation_data(layout, new_element_id);

        sender.submit(info_message::InsertedLogicItemIdUpdated {
            .new_element_id = new_element_id,
            .old_element_id = old_element_id,
            .data = data,
        });
    }

    if (element.is_wire()) {
        const auto& segment_tree = layout.segment_tree(new_element_id);

        for (auto&& segment_index : segment_tree.indices()) {
            sender.submit(info_message::SegmentIdUpdated {
                .new_segment = segment_t {new_element_id, segment_index},
                .old_segment = segment_t {old_element_id, segment_index},
            });
        }
    }

    if (element.is_wire() && inserted) {
        const auto& segment_tree = layout.segment_tree(new_element_id);

        for (auto&& segment_index : segment_tree.indices()) {
            sender.submit(info_message::InsertedSegmentIdUpdated {
                .new_segment = segment_t {new_element_id, segment_index},
                .old_segment = segment_t {old_element_id, segment_index},
                .segment_info = segment_tree.info(segment_index),
            });
        }
    }
}
*/

/*
auto swap_elements(Layout& layout, MessageSender& sender, const element_id_t element_id_0,
                   const element_id_t element_id_1) -> void {
    if (element_id_0 == element_id_1) {
        return;
    }

    if (is_inserted(layout, element_id_0) && is_inserted(layout, element_id_1))
        [[unlikely]] {
        // we might need element delete and uninsert to prevent conflicts
        // or we need to introduce ElementSwapped messages
        throw_exception("not implemented");
    }

    layout.swap_elements(element_id_0, element_id_1);
    notify_element_id_change(layout, sender, element_id_0, element_id_1);
    notify_element_id_change(layout, sender, element_id_1, element_id_0);
}
*/

/*
auto swap_and_delete_single_element_private(Layout& layout, MessageSender& sender,
                                            element_id_t& element_id,
                                            element_id_t* preserve_element = nullptr)
    -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }

    if (layout.display_state(element_id) != display_state_t::temporary) [[unlikely]] {
        throw_exception("can only delete temporary objects");
    }
    if (is_wire_with_segments(layout, element_id)) [[unlikely]] {
        throw_exception("can't delete wires with segments");
    }

    notify_element_deleted(layout, sender, element_id);

    // delete in underlying
    auto last_id = layout.swap_and_delete_element(element_id);

    if (element_id != last_id) {
        notify_element_id_change(layout, sender, element_id, last_id);
    }

    if (preserve_element != nullptr) {
        if (*preserve_element == element_id) {
            *preserve_element = null_element;
        } else if (*preserve_element == last_id) {
            *preserve_element = element_id;
        }
    }

    element_id = null_element;
}

auto swap_and_delete_single_element(Layout& layout, MessageSender& sender,
                                    element_id_t& element_id,
                                    element_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "swap_and_delete_single_element(element_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            layout, element_id, fmt_ptr(preserve_element));
    }
    swap_and_delete_single_element_private(layout, sender, element_id, preserve_element);
}
*/

//
//
//

auto is_wire_with_segments(const Layout& layout, const wire_id_t wire_id) -> bool {
    return !layout.wires().segment_tree(wire_id).empty();
}

auto notify_wire_id_change(const Layout& layout, MessageSender& sender,
                           const wire_id_t new_wire_id, const wire_id_t old_wire_id) {
    const auto& segment_tree = layout.wires().segment_tree(new_wire_id);

    for (auto&& segment_index : segment_tree.indices()) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_t {new_wire_id, segment_index},
            .old_segment = segment_t {old_wire_id, segment_index},
        });
    }

    if (is_inserted(new_wire_id)) {
        for (auto&& segment_index : segment_tree.indices()) {
            sender.submit(info_message::InsertedSegmentIdUpdated {
                .new_segment = segment_t {new_wire_id, segment_index},
                .old_segment = segment_t {old_wire_id, segment_index},
                .segment_info = segment_tree.info(segment_index),
            });
        }
    }
}

auto swap_and_delete_empty_wire_private(Layout& layout, MessageSender& sender,
                                        wire_id_t& wire_id,
                                        wire_id_t* preserve_element = nullptr) -> void {
    if (!wire_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }

    if (!is_inserted(wire_id)) [[unlikely]] {
        throw_exception("can only delete inserted wires");
    }
    if (is_wire_with_segments(layout, wire_id)) [[unlikely]] {
        throw_exception("can't delete wires with segments");
    }

    // delete in underlying
    auto last_id = layout.wires().swap_and_delete(wire_id);

    if (wire_id != last_id) {
        notify_wire_id_change(layout, sender, wire_id, last_id);
    }

    if (preserve_element != nullptr) {
        if (*preserve_element == wire_id) {
            *preserve_element = null_wire_id;
        } else if (*preserve_element == last_id) {
            *preserve_element = wire_id;
        }
    }

    wire_id = null_wire_id;
}

auto swap_and_delete_empty_wire(Layout& layout, MessageSender& sender, wire_id_t& wire_id,
                                wire_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "swap_and_delete_empty_wire(wire_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            layout, wire_id, fmt_ptr(preserve_element));
    }
    swap_and_delete_empty_wire_private(layout, sender, wire_id, preserve_element);
}

//
//
//

auto notify_logic_item_id_change(const Layout& layout, MessageSender& sender,
                                 const logicitem_id_t new_logicitem_id,
                                 const logicitem_id_t old_logicitem_id) {
    sender.submit(info_message::LogicItemIdUpdated {
        .new_logicitem_id = new_logicitem_id,
        .old_logicitem_id = old_logicitem_id,
    });

    if (is_inserted(layout, new_logicitem_id)) {
        const auto data = to_layout_calculation_data(layout, new_logicitem_id);

        sender.submit(info_message::InsertedLogicItemIdUpdated {
            .new_logicitem_id = new_logicitem_id,
            .old_logicitem_id = old_logicitem_id,
            .data = data,
        });
    }
}

auto swap_and_delete_logic_item_private(Layout& layout, MessageSender& sender,
                                        logicitem_id_t& logicitem_id,
                                        logicitem_id_t* preserve_element = nullptr)
    -> void {
    if (!logicitem_id) [[unlikely]] {
        throw_exception("logic item id is invalid");
    }

    if (layout.logic_items().display_state(logicitem_id) != display_state_t::temporary)
        [[unlikely]] {
        throw_exception("can only delete temporary objects");
    }

    sender.submit(info_message::LogicItemDeleted {logicitem_id});

    // delete in underlying
    auto last_id = layout.logic_items().swap_and_delete(logicitem_id);

    if (logicitem_id != last_id) {
        notify_logic_item_id_change(layout, sender, logicitem_id, last_id);
    }

    if (preserve_element != nullptr) {
        if (*preserve_element == logicitem_id) {
            *preserve_element = null_logicitem_id;
        } else if (*preserve_element == last_id) {
            *preserve_element = logicitem_id;
        }
    }

    logicitem_id = null_logicitem_id;
}

auto swap_and_delete_logic_item(Layout& layout, MessageSender& sender,
                                logicitem_id_t& logicitem_id,
                                logicitem_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "swap_and_delete_logic_item(logicitem_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            layout, logicitem_id, fmt_ptr(preserve_element));
    }
    swap_and_delete_logic_item_private(layout, sender, logicitem_id, preserve_element);
}

/*
auto swap_and_delete_multiple_elements_private(Layout& layout, MessageSender& sender,
                                               std::span<const element_id_t> element_ids,
                                               element_id_t* preserve_element) -> void {
    // sort descending, so we don't invalidate our ids
    auto sorted_ids = delete_queue_t {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (auto element_id : sorted_ids) {
        swap_and_delete_single_element_private(layout, sender, element_id,
                                               preserve_element);
    }
}

auto swap_and_delete_multiple_elements(Layout& layout, MessageSender& sender,
                                       std::span<const element_id_t> element_ids,
                                       element_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "swap_and_delete_multiple_elements(element_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            layout, element_ids, fmt_ptr(preserve_element));
    }
    swap_and_delete_multiple_elements_private(layout, sender, element_ids,
                                              preserve_element);
}
*/

//
// Logic Item Handling
//

auto is_logic_item_position_representable_private(const Layout& layout,
                                                  const logicitem_id_t logicitem_id,
                                                  int dx, int dy) -> bool {
    if (!logicitem_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }

    const auto position = layout.logic_items().position(logicitem_id);

    if (!is_representable(position, dx, dy)) {
        return false;
    }

    auto data = to_layout_calculation_data(layout, logicitem_id);
    data.position = add_unchecked(position, dx, dy);

    return is_representable(data);
}

auto is_logic_item_position_representable(const Layout& layout,
                                          const logicitem_id_t logicitem_id, int dx,
                                          int dy) -> bool {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "is_logic_item_position_representable(logicitem_id = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, logicitem_id, dx, dy);
    }
    return is_logic_item_position_representable_private(layout, logicitem_id, dx, dy);
}

auto move_logic_item_unchecked_private(Layout& layout, const logicitem_id_t logicitem_id,
                                       int dx, int dy) -> void {
    const auto position =
        add_unchecked(layout.logic_items().position(logicitem_id), dx, dy);
    layout.logic_items().set_position(logicitem_id, position);
}

auto move_logic_item_unchecked(Layout& layout, const logicitem_id_t logicitem_id, int dx,
                               int dy) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_logic_item_unchecked(logicitem_id = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, logicitem_id, dx, dy);
    }
    move_logic_item_unchecked_private(layout, logicitem_id, dx, dy);
}

auto move_or_delete_logic_item_private(Layout& layout, MessageSender& sender,
                                       logicitem_id_t& logicitem_id, int dx, int dy)
    -> void {
    if (!logicitem_id) [[unlikely]] {
        throw_exception("logicitem id is invalid");
    }
    if (layout.logic_items().display_state(logicitem_id) != display_state_t::temporary)
        [[unlikely]] {
        throw_exception("Only temporary items can be freely moved.");
    }

    if (!is_logic_item_position_representable_private(layout, logicitem_id, dx, dy)) {
        swap_and_delete_logic_item_private(layout, sender, logicitem_id);
        return;
    }

    move_logic_item_unchecked_private(layout, logicitem_id, dx, dy);
}

auto move_or_delete_logic_item(Layout& layout, MessageSender& sender,
                               logicitem_id_t& logicitem_id, int dx, int dy) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_logic_item(logicitem_id = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, logicitem_id, dx, dy);
    }
    move_or_delete_logic_item_private(layout, sender, logicitem_id, dx, dy);
}

auto toggle_inverter_private(Layout& layout, const CacheProvider& cache, point_t point)
    -> void {
    if (const auto entry = cache.logicitem_input_cache().find(point)) {
        const auto layout_data = to_layout_calculation_data(layout, entry->logicitem_id);
        const auto info = input_locations(layout_data).at(entry->connection_id.value);
        assert(info.position == point);

        if (is_directed(info.orientation)) {
            const auto value = layout.logic_items().input_inverted(entry->logicitem_id,
                                                                   entry->connection_id);
            layout.logic_items().set_input_inverter(entry->logicitem_id,
                                                    entry->connection_id, !value);
        }
    }

    if (const auto entry = cache.logicitem_output_cache().find(point)) {
        const auto layout_data = to_layout_calculation_data(layout, entry->logicitem_id);
        const auto info = output_locations(layout_data).at(entry->connection_id.value);
        assert(info.position == point);

        if (is_directed(info.orientation)) {
            const auto value = layout.logic_items().output_inverted(entry->logicitem_id,
                                                                    entry->connection_id);
            layout.logic_items().set_output_inverter(entry->logicitem_id,
                                                     entry->connection_id, !value);
        }
    }
}

auto toggle_inverter(Layout& layout, const CacheProvider& cache, point_t point) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "toggle_inverter(point = {});\n"
            "==========================================================\n\n",
            layout, point);
    }
    toggle_inverter_private(layout, cache, point);
}

//
// logic item mode change
//

auto any_logic_item_inputs_colliding(const CacheProvider& cache,
                                     const layout_calculation_data_t& data) -> bool {
    const auto compatible = [&](simple_input_info_t info) -> bool {
        if (const auto entry = cache.wire_output_cache().find(info.position)) {
            return orientations_compatible(info.orientation, entry->orientation);
        }
        return true;
    };

    return !std::ranges::all_of(input_locations(data), compatible);
}

auto any_logic_item_outputs_colliding(const Layout& layout, const CacheProvider& cache,
                                      const layout_calculation_data_t& data) -> bool {
    return find_convertible_wire_inputs(layout, cache, data).any_collisions;
}

auto is_logic_item_colliding(const Layout& layout, const CacheProvider& cache,
                             const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(layout, logicitem_id);

    return cache.collision_cache().is_colliding(data) ||
           any_logic_item_inputs_colliding(cache, data) ||
           any_logic_item_outputs_colliding(layout, cache, data);
}

auto insert_logic_item_wire_conversion(State state, const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(state.layout, logicitem_id);

    auto result = find_convertible_wire_inputs(state.layout, state.cache, data);

    // we assume there will be no collision at this point
    if (result.any_collisions) [[unlikely]] {
        throw_exception("inserted logic item is colliding");
    }

    convert_to_inputs(state.layout, state.sender, result.convertible_inputs);
}

auto uninsert_logic_item_wire_conversion(State state, const logicitem_id_t logicitem_id)
    -> void {
    const auto data = to_layout_calculation_data(state.layout, logicitem_id);

    for (auto info : output_locations(data)) {
        if (const auto entry = state.cache.wire_input_cache().find(info.position)) {
            const auto connection = wire_connection_t {info.position, entry->segment};
            convert_to_output(state.layout, state.sender, connection);
        }
    }
}

auto notify_logic_item_inserted(const Layout& layout, MessageSender& sender,
                                const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(layout, logicitem_id);
    sender.submit(info_message::LogicItemInserted {logicitem_id, data});
}

auto notify_logic_item_uninserted(const Layout& layout, MessageSender& sender,
                                  const logicitem_id_t logicitem_id) {
    const auto data = to_layout_calculation_data(layout, logicitem_id);
    sender.submit(info_message::LogicItemUninserted {logicitem_id, data});
}

auto _element_change_temporary_to_colliding(State state,
                                            const logicitem_id_t logicitem_id) -> void {
    if (state.layout.logic_items().display_state(logicitem_id) !=
        display_state_t::temporary) [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    if (is_logic_item_colliding(state.layout, state.cache, logicitem_id)) {
        state.layout.logic_items().set_display_state(logicitem_id,
                                                     display_state_t::colliding);
    } else {
        insert_logic_item_wire_conversion(state, logicitem_id);
        state.layout.logic_items().set_display_state(logicitem_id,
                                                     display_state_t::valid);
        notify_logic_item_inserted(state.layout, state.sender, logicitem_id);
    }
};

auto _element_change_colliding_to_insert(Layout& layout, MessageSender& sender,
                                         logicitem_id_t& logicitem_id) -> void {
    const auto display_state = layout.logic_items().display_state(logicitem_id);

    if (display_state == display_state_t::valid) {
        layout.logic_items().set_display_state(logicitem_id, display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::colliding) [[likely]] {
        // we can only delete temporary elements
        layout.logic_items().set_display_state(logicitem_id, display_state_t::temporary);
        swap_and_delete_logic_item_private(layout, sender, logicitem_id);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto _element_change_insert_to_colliding(Layout& layout,
                                         const logicitem_id_t logicitem_id) -> void {
    if (layout.logic_items().display_state(logicitem_id) != display_state_t::normal)
        [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    layout.logic_items().set_display_state(logicitem_id, display_state_t::valid);
};

auto _element_change_colliding_to_temporary(State state,
                                            const logicitem_id_t logicitem_id) -> void {
    const auto display_state = state.layout.logic_items().display_state(logicitem_id);

    if (display_state == display_state_t::valid) {
        notify_logic_item_uninserted(state.layout, state.sender, logicitem_id);
        state.layout.logic_items().set_display_state(logicitem_id,
                                                     display_state_t::temporary);
        uninsert_logic_item_wire_conversion(state, logicitem_id);
        return;
    }

    if (display_state == display_state_t::colliding) {
        state.layout.logic_items().set_display_state(logicitem_id,
                                                     display_state_t::temporary);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto change_logic_item_insertion_mode_private(State state, logicitem_id_t& logicitem_id,
                                              InsertionMode new_mode) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }

    const auto old_mode =
        to_insertion_mode(state.layout.logic_items().display_state(logicitem_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        _element_change_temporary_to_colliding(state, logicitem_id);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _element_change_colliding_to_insert(state.layout, state.sender, logicitem_id);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        _element_change_insert_to_colliding(state.layout, logicitem_id);
    }
    if (new_mode == InsertionMode::temporary) {
        _element_change_colliding_to_temporary(state, logicitem_id);
    }
}

auto change_logic_item_insertion_mode(State state, logicitem_id_t& logicitem_id,
                                      InsertionMode new_mode) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_logic_item_insertion_mode(logicitem_id = {}, new_mode = {});\n"
            "==========================================================\n\n",
            state.layout, logicitem_id, new_mode);
    }
    change_logic_item_insertion_mode_private(state, logicitem_id, new_mode);
}

auto add_logic_item_private(State state, const ElementDefinition& definition,
                            point_t position, InsertionMode insertion_mode)
    -> logicitem_id_t {
    // insert into underlying
    auto logicitem_id = state.layout.logic_items().add_logicitem(
        definition, point_t {0, 0}, display_state_t::temporary);
    state.sender.submit(info_message::LogicItemCreated {logicitem_id});

    // validates our position
    move_or_delete_logic_item_private(state.layout, state.sender, logicitem_id,  //
                                      int {position.x},                          //
                                      int {position.y});
    if (logicitem_id) {
        change_logic_item_insertion_mode_private(state, logicitem_id, insertion_mode);
    }
    return logicitem_id;
}

auto add_logic_item(State state, const ElementDefinition& definition, point_t position,
                    InsertionMode insertion_mode) -> logicitem_id_t {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_logic_item(definition = {}, position = {}, insertion_mode = {});\n"
            "==========================================================\n\n",
            state.layout, definition, position, insertion_mode);
    }
    return add_logic_item_private(state, definition, position, insertion_mode);
}

//
// Wire Handling
//

// aggregates

/*
auto is_wire_aggregate(const Layout& layout, const element_id_t element_id,
                       display_state_t display_state) -> bool {
    const auto element = layout.element(element_id);
    return element.is_wire() && element.display_state() == display_state;
}

auto add_new_wire_element(Layout& layout, display_state_t display_state) -> element_id_t {
    const auto definition = ElementDefinition {
        .element_type = ElementType::wire,

        .input_count = connection_count_t {0},
        .output_count = connection_count_t {0},
    };

    return layout.add_element(definition, point_t {}, display_state).element_id();
}

auto find_wire(const Layout& layout, display_state_t display_state) -> element_id_t {
    const auto element_ids = layout.element_ids();
    const auto it =
        std::ranges::find_if(element_ids, [&](element_id_t element_id) -> bool {
            return is_wire_aggregate(layout, element_id, display_state);
        });
    return it == element_ids.end() ? null_element : *it;
}

auto create_aggregate_tree_at(Layout& layout, MessageSender& sender,
                              display_state_t display_state, const element_id_t target_id)
    -> void {
    auto element_id = find_wire(layout, display_state);

    if (!element_id) {
        element_id = add_new_wire_element(layout, display_state);
    }

    if (element_id != target_id) {
        swap_elements(layout, sender, element_id, target_id);
    }
}

constexpr inline static auto TEMPORARY_AGGREGATE_ID = element_id_t {0};
constexpr inline static auto COLLIDING_AGGREGATE_ID = element_id_t {1};

auto create_aggregate_wires(Layout& layout, MessageSender& sender) -> void {
    using enum display_state_t;
    create_aggregate_tree_at(layout, sender, temporary, TEMPORARY_AGGREGATE_ID);
    create_aggregate_tree_at(layout, sender, colliding, COLLIDING_AGGREGATE_ID);
}

auto get_or_create_aggregate(Layout& layout, MessageSender& sender,
                             display_state_t display_state) -> element_id_t {
    using enum display_state_t;

    // temporary
    if (display_state == temporary) {
        if (layout.element_count() <= std::size_t {TEMPORARY_AGGREGATE_ID} ||
            !is_wire_aggregate(layout, TEMPORARY_AGGREGATE_ID, temporary)) {
            create_aggregate_wires(layout, sender);
        }
        return TEMPORARY_AGGREGATE_ID;
    }

    // colliding
    else if (display_state == colliding) {
        if (layout.element_count() <= std::size_t {COLLIDING_AGGREGATE_ID} ||
            !is_wire_aggregate(layout, COLLIDING_AGGREGATE_ID, temporary)) {
            create_aggregate_wires(layout, sender);
        }
        return COLLIDING_AGGREGATE_ID;
    }

    throw_exception("display state has no aggregate");
}
*/

auto add_new_wire_element(Layout& layout) -> wire_id_t {
    return layout.wires().add_wire();
}

auto add_segment_to_tree(Layout& layout, MessageSender& sender, const wire_id_t wire_id,
                         ordered_line_t line) -> segment_part_t {
    // insert new segment
    auto& m_tree = layout.wires().modifyable_segment_tree(wire_id);

    const auto segment_info = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };
    const auto segment_index = m_tree.add_segment(segment_info);
    const auto segment = segment_t {wire_id, segment_index};

    // messages
    sender.submit(info_message::SegmentCreated {segment});
    if (is_inserted(wire_id)) {
        sender.submit(info_message::SegmentInserted {segment, segment_info});
    }

    return segment_part_t {segment, to_part(line)};
}

auto reset_segment_endpoints(Layout& layout, const segment_t segment) {
    if (is_inserted(segment.wire_id)) [[unlikely]] {
        throw_exception("cannot reset endpoints of inserted wire segment");
    }
    auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);

    const auto new_info = segment_info_t {
        .line = m_tree.line(segment.segment_index),
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };

    m_tree.update_segment(segment.segment_index, new_info);
}

auto set_segment_crosspoint(Layout& layout, const segment_t segment, point_t point) {
    if (is_inserted(segment.wire_id)) [[unlikely]] {
        throw_exception("cannot set endpoints of inserted wire segment");
    }
    auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);

    auto info = m_tree.info(segment.segment_index);

    if (info.line.p0 == point) {
        info.p0_type = SegmentPointType::cross_point;
    } else if (info.line.p1 == point) {
        info.p1_type = SegmentPointType::cross_point;
    } else [[unlikely]] {
        throw_exception("point is not part of line.");
    }

    m_tree.update_segment(segment.segment_index, info);
}

// auto add_segment_to_aggregate(Layout& layout, MessageSender& sender,
//                               const ordered_line_t line,
//                               const display_state_t aggregate_type) -> segment_part_t {
//     const auto element_id = get_or_create_aggregate(layout, sender, aggregate_type);
//     return add_segment_to_tree(layout, sender, element_id, line);
// }

//
// wire insertion mode changing
//

auto wire_endpoints_colliding(const Layout& layout, const CacheProvider& cache,
                              ordered_line_t line) -> bool {
    const auto wire_id_0 = cache.collision_cache().get_first_wire(line.p0);
    const auto wire_id_1 = cache.collision_cache().get_first_wire(line.p1);

    // loop check
    if (wire_id_0 && wire_id_0 == wire_id_1) {
        return true;
    }

    // count existing inputs
    auto input_count = 0;
    if (wire_id_0 && layout.wires().segment_tree(wire_id_0).has_input()) {
        ++input_count;
    }
    if (wire_id_1 && layout.wires().segment_tree(wire_id_1).has_input()) {
        ++input_count;
    }
    if (input_count > 1) {
        return true;
    }

    // check for LogicItem Outputs  (requires additional inputs)
    if (!wire_id_0) {
        if (const auto entry = cache.logicitem_output_cache().find(line.p0)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p0(line))) {
                return true;
            }
            ++input_count;
        }
    }
    if (!wire_id_1) {
        if (const auto entry = cache.logicitem_output_cache().find(line.p1)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p1(line))) {
                return true;
            }
            ++input_count;
        }
    }
    if (input_count > 1) {
        return true;
    }

    // check for LogicItem Inputs
    if (!wire_id_0) {
        if (const auto entry = cache.logicitem_input_cache().find(line.p0)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p0(line))) {
                return true;
            }
        }
    }
    if (!wire_id_1) {
        if (const auto entry = cache.logicitem_input_cache().find(line.p1)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p1(line))) {
                return true;
            }
        }
    }

    return false;
}

auto is_wire_colliding(const Layout& layout, const CacheProvider& cache,
                       const ordered_line_t line) -> bool {
    return wire_endpoints_colliding(layout, cache, line) ||
           cache.collision_cache().is_colliding(line);
}

auto get_display_states(const Layout& layout, const segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t> {
    using enum display_state_t;

    const auto& tree = layout.wires().segment_tree(segment_part.segment.wire_id);
    const auto tree_state = to_display_state(segment_part.segment.wire_id);

    // aggregates
    if (tree_state == temporary || tree_state == colliding) {
        return std::make_pair(tree_state, tree_state);
    }

    // check valid parts
    for (const auto valid_part : tree.valid_parts(segment_part.segment.segment_index)) {
        // parts can not touch or overlap, so we can return early
        if (a_inside_b(segment_part.part, valid_part)) {
            return std::make_pair(valid, valid);
        }
        if (a_overlapps_any_of_b(segment_part.part, valid_part)) {
            return std::make_pair(valid, normal);
        }
    }
    return std::make_pair(normal, normal);
}

auto get_insertion_modes(const Layout& layout, const segment_part_t segment_part)
    -> std::pair<InsertionMode, InsertionMode> {
    const auto display_states = get_display_states(layout, segment_part);

    return std::make_pair(to_insertion_mode(display_states.first),
                          to_insertion_mode(display_states.second));
}

// segment already moved
auto notify_segment_insertion_status_changed(Layout& layout, MessageSender& sender,
                                             const segment_t source_segment,
                                             const segment_t destination_segment,
                                             const segment_t last_segment) {
    const auto source_inserted = is_inserted(source_segment.wire_id);
    const auto destination_inserted = is_inserted(destination_segment.wire_id);

    const auto info = get_segment_info(layout, destination_segment);

    // insertion / un-insertion
    if (source_inserted && destination_inserted) {
        sender.submit(info_message::InsertedSegmentIdUpdated({
            .new_segment = destination_segment,
            .old_segment = source_segment,
            .segment_info = info,
        }));
    }
    if (source_inserted && !destination_inserted) {
        sender.submit(info_message::SegmentUninserted({
            .segment = source_segment,
            .segment_info = info,
        }));
    }
    if (destination_inserted && !source_inserted) {
        sender.submit(info_message::SegmentInserted({
            .segment = destination_segment,
            .segment_info = info,
        }));
    }

    // another element swapped
    if (last_segment != source_segment && source_inserted) {
        sender.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = source_segment,
            .old_segment = last_segment,
            .segment_info = get_segment_info(layout, source_segment),
        });
    }
}

// segment already moved
auto notify_segment_id_changed(MessageSender& sender, const segment_t source_segment,
                               const segment_t destination_segment,
                               const segment_t last_segment) {
    sender.submit(info_message::SegmentIdUpdated {
        .new_segment = destination_segment,
        .old_segment = source_segment,
    });

    // another element swapped
    if (last_segment != source_segment) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = source_segment,
            .old_segment = last_segment,
        });
    }
}

auto _move_full_segment_between_trees(Layout& layout, MessageSender& sender,
                                      segment_t& source_segment,
                                      const wire_id_t destination_id) {
    if (source_segment.wire_id == destination_id) {
        return;
    }
    const auto source_index = source_segment.segment_index;

    auto& m_tree_source = layout.wires().modifyable_segment_tree(source_segment.wire_id);
    auto& m_tree_destination = layout.wires().modifyable_segment_tree(destination_id);

    // copy
    const auto destination_index =
        m_tree_destination.copy_segment(m_tree_source, source_index);
    const auto last_index = m_tree_source.last_index();
    m_tree_source.swap_and_delete_segment(source_index);

    // messages
    const auto destination_segment = segment_t {destination_id, destination_index};
    const auto last_segment = segment_t {source_segment.wire_id, last_index};

    notify_segment_id_changed(sender, source_segment, destination_segment, last_segment);
    notify_segment_insertion_status_changed(layout, sender, source_segment,
                                            destination_segment, last_segment);

    source_segment = destination_segment;
}

namespace detail::move_segment {

auto copy_segment(Layout& layout, MessageSender& sender,
                  const segment_part_t source_segment_part,
                  const wire_id_t destination_id) -> segment_part_t {
    auto& m_tree_source =
        layout.wires().modifyable_segment_tree(source_segment_part.segment.wire_id);
    auto& m_tree_destination = layout.wires().modifyable_segment_tree(destination_id);

    bool set_input_p0 = false;
    bool set_input_p1 = false;
    // handle inputs being copied within the same tree
    {
        if (destination_id == source_segment_part.segment.wire_id) {
            auto info = m_tree_source.info(source_segment_part.segment.segment_index);
            const auto full_part = to_part(info.line);

            if (full_part.begin == source_segment_part.part.begin &&
                info.p0_type == SegmentPointType::input) {
                info.p0_type = SegmentPointType::shadow_point;
                m_tree_source.update_segment(source_segment_part.segment.segment_index,
                                             info);
                set_input_p0 = true;
            }
            if (full_part.end == source_segment_part.part.end &&
                info.p1_type == SegmentPointType::input) {
                info.p1_type = SegmentPointType::shadow_point;
                m_tree_source.update_segment(source_segment_part.segment.segment_index,
                                             info);
                set_input_p1 = true;
            }
        }
    }

    const auto destination_index = m_tree_destination.copy_segment(
        m_tree_source, source_segment_part.segment.segment_index,
        source_segment_part.part);

    const auto destination_segment_part =
        segment_part_t {segment_t {destination_id, destination_index},
                        m_tree_destination.part(destination_index)};

    {
        if (set_input_p0) {
            auto info = m_tree_destination.info(destination_index);
            info.p0_type = SegmentPointType::input;
            m_tree_destination.update_segment(destination_index, info);
        }
        if (set_input_p1) {
            auto info = m_tree_destination.info(destination_index);
            info.p1_type = SegmentPointType::input;
            m_tree_destination.update_segment(destination_index, info);
        }
    }

    sender.submit(info_message::SegmentCreated {destination_segment_part.segment});

    if (is_inserted(destination_id)) {
        sender.submit(info_message::SegmentInserted({
            .segment = destination_segment_part.segment,
            .segment_info = get_segment_info(layout, destination_segment_part.segment),
        }));
    }

    return destination_segment_part;
}

auto shrink_segment_begin(Layout& layout, MessageSender& sender, const segment_t segment)
    -> void {
    using namespace info_message;

    if (is_inserted(segment.wire_id)) {
        auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);
        const auto old_info = m_tree.info(segment.segment_index);
        sender.submit(SegmentUninserted({.segment = segment, .segment_info = old_info}));
    }
}

auto shrink_segment_end(Layout& layout, MessageSender& sender, const segment_t segment,
                        const part_t part_kept) -> segment_part_t {
    using namespace info_message;
    auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);
    m_tree.shrink_segment(segment.segment_index, part_kept);

    if (is_inserted(segment.wire_id)) {
        const auto new_info = m_tree.info(segment.segment_index);
        sender.submit(SegmentInserted({.segment = segment, .segment_info = new_info}));
    }

    return segment_part_t {
        .segment = segment,
        .part = m_tree.part(segment.segment_index),
    };
}

}  // namespace detail::move_segment

auto _move_touching_segment_between_trees(Layout& layout, MessageSender& sender,
                                          segment_part_t& source_segment_part,
                                          const wire_id_t destination_id) {
    const auto full_part = to_part(get_line(layout, source_segment_part.segment));
    const auto part_kept =
        difference_touching_one_side(full_part, source_segment_part.part);

    // move
    detail::move_segment::shrink_segment_begin(layout, sender,
                                               source_segment_part.segment);
    const auto destination_segment_part = detail::move_segment::copy_segment(
        layout, sender, source_segment_part, destination_id);
    const auto leftover_segment_part = detail::move_segment::shrink_segment_end(
        layout, sender, source_segment_part.segment, part_kept);

    // messages
    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination = destination_segment_part,
        .segment_part_source = source_segment_part,
    });

    if (part_kept.begin != full_part.begin) {
        sender.submit(info_message::SegmentPartMoved {
            .segment_part_destination = leftover_segment_part,
            .segment_part_source = segment_part_t {.segment = source_segment_part.segment,
                                                   .part = part_kept},
        });
    }

    source_segment_part = destination_segment_part;
}

auto _move_splitting_segment_between_trees(Layout& layout, MessageSender& sender,
                                           segment_part_t& source_segment_part,
                                           const wire_id_t destination_id) {
    const auto full_part = to_part(get_line(layout, source_segment_part.segment));
    const auto [part0, part1] =
        difference_not_touching(full_part, source_segment_part.part);

    // move
    const auto source_part1 = segment_part_t {source_segment_part.segment, part1};

    detail::move_segment::shrink_segment_begin(layout, sender,
                                               source_segment_part.segment);
    const auto destination_part1 = detail::move_segment::copy_segment(
        layout, sender, source_part1, source_part1.segment.wire_id);
    const auto destination_segment_part = detail::move_segment::copy_segment(
        layout, sender, source_segment_part, destination_id);
    detail::move_segment::shrink_segment_end(layout, sender, source_segment_part.segment,
                                             part0);

    // messages
    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination = destination_part1,
        .segment_part_source = source_part1,
    });

    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination = destination_segment_part,
        .segment_part_source = source_segment_part,
    });

    source_segment_part = destination_segment_part;
}

//  * trees can become empty
//  * inserts new endpoints as shaddow points
auto move_segment_between_trees(Layout& layout, MessageSender& sender,
                                segment_part_t& segment_part,
                                const wire_id_t destination_id) -> void {
    const auto moving_part = segment_part.part;
    const auto full_line = get_line(layout, segment_part.segment);
    const auto full_part = to_part(full_line);

    if (a_equal_b(moving_part, full_part)) {
        _move_full_segment_between_trees(layout, sender, segment_part.segment,
                                         destination_id);
    } else if (a_inside_b_touching_one_side(moving_part, full_part)) {
        _move_touching_segment_between_trees(layout, sender, segment_part,
                                             destination_id);
    } else if (a_inside_b_not_touching(moving_part, full_part)) {
        _move_splitting_segment_between_trees(layout, sender, segment_part,
                                              destination_id);
    } else {
        throw_exception("segment part is invalid");
    }
}

auto _remove_full_segment_from_tree(Layout& layout, MessageSender& sender,
                                    segment_part_t& full_segment_part) {
    const auto wire_id = full_segment_part.segment.wire_id;
    const auto segment_index = full_segment_part.segment.segment_index;
    auto& m_tree = layout.wires().modifyable_segment_tree(wire_id);

    // delete
    const auto last_index = m_tree.last_index();
    m_tree.swap_and_delete_segment(segment_index);

    // messages
    sender.submit(info_message::SegmentPartDeleted {full_segment_part});

    if (last_index != segment_index) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_t {wire_id, segment_index},
            .old_segment = segment_t {wire_id, last_index},
        });
    }

    full_segment_part = null_segment_part;
}

auto _remove_touching_segment_from_tree(Layout& layout, MessageSender& sender,
                                        segment_part_t& segment_part) {
    const auto wire_id = segment_part.segment.wire_id;
    const auto index = segment_part.segment.segment_index;
    const auto part = segment_part.part;

    auto& m_tree = layout.wires().modifyable_segment_tree(wire_id);

    const auto full_part = m_tree.part(index);
    const auto part_kept = difference_touching_one_side(full_part, part);

    // delete
    m_tree.shrink_segment(index, part_kept);

    // messages
    sender.submit(info_message::SegmentPartDeleted {segment_part});

    if (part_kept.begin != full_part.begin) {
        sender.submit(info_message::SegmentPartMoved {
            .segment_part_destination = segment_part_t {.segment = segment_part.segment,
                                                        .part = m_tree.part(index)},
            .segment_part_source =
                segment_part_t {.segment = segment_part.segment, .part = part_kept},
        });
    }

    segment_part = null_segment_part;
}

auto _remove_splitting_segment_from_tree(Layout& layout, MessageSender& sender,
                                         segment_part_t& segment_part) {
    const auto wire_id = segment_part.segment.wire_id;
    const auto index = segment_part.segment.segment_index;
    const auto part = segment_part.part;

    auto& m_tree = layout.wires().modifyable_segment_tree(wire_id);

    const auto full_part = m_tree.part(index);
    const auto [part0, part1] = difference_not_touching(full_part, part);

    // delete
    const auto index1 = m_tree.copy_segment(m_tree, index, part1);
    m_tree.shrink_segment(index, part0);

    // messages
    const auto segment_part_1 =
        segment_part_t {segment_t {wire_id, index1}, m_tree.part(index1)};

    sender.submit(info_message::SegmentCreated {segment_part_1.segment});

    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination = segment_part_1,
        .segment_part_source = segment_part_t {segment_part.segment, part1}});

    sender.submit(info_message::SegmentPartDeleted {segment_part});

    segment_part = null_segment_part;
}

//  * trees can become empty
//  * inserts new endpoints as shadow points
//  * will not send insert / uninserted messages
auto remove_segment_from_tree(Layout& layout, MessageSender& sender,
                              segment_part_t& segment_part) -> void {
    if (is_inserted(segment_part.segment.wire_id)) [[unlikely]] {
        throw_exception("can only remove from non-inserted segments");
    }

    const auto removed_part = segment_part.part;
    const auto full_line = get_line(layout, segment_part.segment);
    const auto full_part = to_part(full_line);

    if (a_equal_b(removed_part, full_part)) {
        _remove_full_segment_from_tree(layout, sender, segment_part);
    } else if (a_inside_b_touching_one_side(removed_part, full_part)) {
        _remove_touching_segment_from_tree(layout, sender, segment_part);
    } else if (a_inside_b_not_touching(removed_part, full_part)) {
        _remove_splitting_segment_from_tree(layout, sender, segment_part);
    } else {
        throw_exception("segment part is invalid");
    }
}

auto merge_and_delete_tree(Layout& layout, MessageSender& sender,
                           wire_id_t& tree_destination, wire_id_t& tree_source) -> void {
    if (tree_destination >= tree_source) [[unlikely]] {
        // optimization
        throw_exception("source is deleted and should have larget id");
    }

    if (!is_inserted(tree_source) && !is_inserted(tree_destination)) [[unlikely]] {
        throw_exception("only supports merging of inserted trees");
    }

    auto& m_tree_source = layout.wires().modifyable_segment_tree(tree_source);
    auto& m_tree_destination = layout.wires().modifyable_segment_tree(tree_destination);

    auto new_index = m_tree_destination.last_index();

    for (auto old_index : m_tree_source.indices()) {
        const auto segment_info = m_tree_source.info(old_index);
        ++new_index;

        const auto old_segment = segment_t {tree_source, old_index};
        const auto new_segment = segment_t {tree_destination, new_index};

        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = new_segment,
            .old_segment = old_segment,
        });
        sender.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = new_segment,
            .old_segment = old_segment,
            .segment_info = segment_info,
        });
    }

    m_tree_destination.add_tree(m_tree_source);

    m_tree_source.clear();
    swap_and_delete_empty_wire_private(layout, sender, tree_source, &tree_destination);
}

auto updated_segment_info(segment_info_t segment_info, const point_t position,
                          const SegmentPointType point_type) -> segment_info_t {
    if (segment_info.line.p0 == position) {
        segment_info.p0_type = point_type;
    } else if (segment_info.line.p1 == position) {
        segment_info.p1_type = point_type;
    } else {
        throw_exception("Position needs to be an endpoint of the segment.");
    }
    return segment_info;
}

using point_update_t =
    std::initializer_list<const std::pair<segment_index_t, SegmentPointType>>;

auto update_segment_point_types(Layout& layout, MessageSender& sender, wire_id_t wire_id,
                                point_update_t data, const point_t position) -> void {
    if (data.size() == 0) {
        return;
    }
    if (!is_inserted(wire_id)) [[unlikely]] {
        throw_exception("only works for inserted segment trees.");
    }
    auto& m_tree = layout.wires().modifyable_segment_tree(wire_id);

    const auto run_point_update = [&](bool set_to_shadow) {
        for (auto [segment_index, point_type] : data) {
            const auto old_info = m_tree.info(segment_index);
            const auto new_info = updated_segment_info(
                old_info, position,
                set_to_shadow ? SegmentPointType::shadow_point : point_type);

            if (old_info != new_info) {
                m_tree.update_segment(segment_index, new_info);

                sender.submit(info_message::InsertedEndPointsUpdated {
                    .segment = segment_t {wire_id, segment_index},
                    .new_segment_info = new_info,
                    .old_segment_info = old_info,
                });
            }
        }
    };

    // first empty caches
    run_point_update(true);
    // write the new states
    run_point_update(false);
}

auto sort_through_lines_first(std::span<std::pair<ordered_line_t, segment_index_t>> lines,
                              const point_t point) -> void {
    std::ranges::sort(lines, {},
                      [point](std::pair<ordered_line_t, segment_index_t> item) {
                          return is_endpoint(point, item.first);
                      });
}

auto _merge_line_segments_ordered(Layout& layout, MessageSender& sender,
                                  const segment_t segment_0, const segment_t segment_1,
                                  segment_part_t* preserve_segment) -> void {
    if (segment_0.wire_id != segment_1.wire_id) [[unlikely]] {
        throw_exception("Cannot merge segments of different trees.");
    }
    if (segment_0.segment_index >= segment_1.segment_index) [[unlikely]] {
        throw_exception("Segment indices need to be ordered and not the same.");
    }
    const auto is_inserted = ::logicsim::is_inserted(segment_0.wire_id);

    const auto index_0 = segment_0.segment_index;
    const auto index_1 = segment_1.segment_index;
    const auto wire_id = segment_0.wire_id;

    auto& m_tree = layout.wires().modifyable_segment_tree(wire_id);
    const auto index_last = m_tree.last_index();
    const auto segment_last = segment_t {wire_id, index_last};

    const auto info_0 = m_tree.info(index_0);
    const auto info_1 = m_tree.info(index_1);

    // merge
    m_tree.swap_and_merge_segment({.index_merge_to = index_0, .index_deleted = index_1});
    const auto info_merged = m_tree.info(index_0);

    // messages
    if (is_inserted) {
        sender.submit(info_message::SegmentUninserted {segment_0, info_0});
        sender.submit(info_message::SegmentUninserted {segment_1, info_1});
        sender.submit(info_message::SegmentInserted {segment_0, info_merged});
    }

    if (to_part(info_0.line) != to_part(info_merged.line, info_0.line)) {
        sender.submit(info_message::SegmentPartMoved {
            .segment_part_destination =
                segment_part_t {segment_0, to_part(info_merged.line, info_0.line)},
            .segment_part_source = segment_part_t {segment_0, to_part(info_0.line)},
        });
    }

    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_0, to_part(info_merged.line, info_1.line)},
        .segment_part_source = segment_part_t {segment_1, to_part(info_1.line)},
    });

    if (index_1 != index_last) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
        });
        if (is_inserted) {
            sender.submit(info_message::InsertedSegmentIdUpdated {
                .new_segment = segment_1,
                .old_segment = segment_last,
                .segment_info = m_tree.info(index_1),
            });
        }
    }

    // preserve
    if (preserve_segment && preserve_segment->segment.wire_id == wire_id) {
        const auto p_index = preserve_segment->segment.segment_index;

        if (p_index == index_0 || p_index == index_1) {
            const auto p_info = p_index == index_0 ? info_0 : info_1;
            const auto p_line = to_line(p_info.line, preserve_segment->part);
            const auto p_part = to_part(info_merged.line, p_line);
            *preserve_segment = segment_part_t {segment_t {wire_id, index_0}, p_part};
        }

        else if (p_index == index_last) {
            const auto p_part = preserve_segment->part;
            *preserve_segment = segment_part_t {segment_t {wire_id, index_1}, p_part};
        }
    }
}

auto merge_line_segments(Layout& layout, MessageSender& sender, segment_t segment_0,
                         segment_t segment_1, segment_part_t* preserve_segment) -> void {
    if (segment_0.segment_index < segment_1.segment_index) {
        _merge_line_segments_ordered(layout, sender, segment_0, segment_1,
                                     preserve_segment);
    } else {
        _merge_line_segments_ordered(layout, sender, segment_1, segment_0,
                                     preserve_segment);
    }
}

auto merge_all_line_segments(Layout& layout, MessageSender& sender,
                             std::vector<std::pair<segment_t, segment_t>>& pairs) {
    // merging deletes the segment with highest segment index,
    // so for this to work with multiple segments
    // we need to be sort them in descendant order
    for (auto& pair : pairs) {
        sort_inplace(pair.first, pair.second, std::ranges::greater {});
    }
    std::ranges::sort(pairs, std::ranges::greater {});

    // Sorted pairs example:
    //  (<Element 0, Segment 6>, <Element 0, Segment 5>)
    //  (<Element 0, Segment 5>, <Element 0, Segment 3>)
    //  (<Element 0, Segment 4>, <Element 0, Segment 2>)
    //  (<Element 0, Segment 4>, <Element 0, Segment 0>)  <-- 4 needs to become 2
    //  (<Element 0, Segment 3>, <Element 0, Segment 1>)
    //  (<Element 0, Segment 2>, <Element 0, Segment 1>)
    //                                                    <-- move here & become 1

    for (auto it = pairs.begin(); it != pairs.end(); ++it) {
        merge_line_segments(layout, sender, it->first, it->second, nullptr);

        const auto other = std::ranges::lower_bound(
            std::next(it), pairs.end(), it->first, std::ranges::greater {},
            [](std::pair<segment_t, segment_t> pair) { return pair.first; });

        if (other != pairs.end() && other->first == it->first) {
            other->first = it->second;

            sort_inplace(other->first, other->second, std::ranges::greater {});
            std::ranges::sort(std::next(it), pairs.end(), std::ranges::greater {});
        }
    }
}

auto split_line_segment(Layout& layout, MessageSender& sender, const segment_t segment,
                        const point_t position) -> segment_part_t {
    const auto full_line = get_line(layout, segment);
    const auto line_moved = ordered_line_t {position, full_line.p1};

    auto move_segment_part = segment_part_t {segment, to_part(full_line, line_moved)};
    move_segment_between_trees(layout, sender, move_segment_part, segment.wire_id);

    return move_segment_part;
}

auto fix_and_merge_segments(State state, const point_t position,
                            segment_part_t* preserve_segment = nullptr) -> void {
    auto& layout = state.layout;

    const auto segments = state.cache.spatial_cache().query_line_segments(position);
    const auto segment_count = get_segment_count(segments);

    if (segment_count == 0) [[unlikely]] {
        return;
        // throw_exception("Could not find any segments at position.");
    }
    const auto wire_id = get_unique_wire_id(segments);
    const auto indices = get_segment_indices(segments);

    if (segment_count == 1) {
        const auto new_type = get_segment_point_type(layout, segments.at(0), position) ==
                                      SegmentPointType::input
                                  ? SegmentPointType::input
                                  : SegmentPointType::output;

        update_segment_point_types(state.layout, state.sender, wire_id,
                                   {
                                       std::pair {indices.at(0), new_type},
                                   },
                                   position);

        return;
    }

    if (segment_count == 2) {
        auto lines = std::array {
            std::pair {get_line(layout, segments.at(0)), indices.at(0)},
            std::pair {get_line(layout, segments.at(1)), indices.at(1)},
        };
        sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            split_line_segment(state.layout, state.sender,
                               segment_t {wire_id, lines.at(0).second}, position);
            fix_and_merge_segments(state, position, preserve_segment);
            return;
        }

        const auto horizontal_0 = is_horizontal(lines.at(0).first);
        const auto horizontal_1 = is_horizontal(lines.at(1).first);
        const auto parallel = horizontal_0 == horizontal_1;

        if (parallel) {
            merge_line_segments(state.layout, state.sender, segments.at(0),
                                segments.at(1), preserve_segment);
            return;
        }

        // this handles corners
        update_segment_point_types(
            state.layout, state.sender, wire_id,
            {
                std::pair {indices.at(0), SegmentPointType::corner_point},
                std::pair {indices.at(1), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    if (segment_count == 3) {
        auto lines = std::array {
            std::pair {get_line(layout, segments.at(0)), indices.at(0)},
            std::pair {get_line(layout, segments.at(1)), indices.at(1)},
            std::pair {get_line(layout, segments.at(2)), indices.at(2)},
        };
        sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            throw_exception("This is not allowed, segment should have been splitted");
        } else {
            update_segment_point_types(
                state.layout, state.sender, wire_id,
                {
                    std::pair {indices.at(0), SegmentPointType::cross_point},
                    std::pair {indices.at(1), SegmentPointType::shadow_point},
                    std::pair {indices.at(2), SegmentPointType::shadow_point},
                },
                position);
        }
        return;
    }

    if (segment_count == 4) {
        update_segment_point_types(
            state.layout, state.sender, wire_id,
            {
                std::pair {indices.at(0), SegmentPointType::cross_point},
                std::pair {indices.at(1), SegmentPointType::shadow_point},
                std::pair {indices.at(2), SegmentPointType::shadow_point},
                std::pair {indices.at(3), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    throw_exception("unexpected unhandeled case");
}

auto find_wire_for_inserting_segment(State state, const segment_part_t segment_part)
    -> wire_id_t {
    const auto line = get_line(state.layout, segment_part);

    auto candidate_0 = state.cache.collision_cache().get_first_wire(line.p0);
    auto candidate_1 = state.cache.collision_cache().get_first_wire(line.p1);

    // 1 wire
    if (bool {candidate_0} ^ bool {candidate_1}) {
        return candidate_0 ? candidate_0 : candidate_1;
    }

    // 2 wires
    if (candidate_0 && candidate_1) {
        // we assume segment is part of aggregates that have ID 0 and 1
        if (segment_part.segment.wire_id > candidate_0 ||
            segment_part.segment.wire_id > candidate_1) {
            throw_exception("cannot preserve segment wire_id");
        }

        if (candidate_0 > candidate_1) {
            using std::swap;
            swap(candidate_0, candidate_1);
        }

        merge_and_delete_tree(state.layout, state.sender, candidate_0, candidate_1);
        return candidate_0;
    }

    // 0 wires
    return add_new_wire_element(state.layout);
}

auto discover_wire_inputs(Layout& layout, const CacheProvider& cache, segment_t segment) {
    const auto line = get_line(layout, segment);

    // find LogicItem outputs
    if (const auto entry = cache.logicitem_output_cache().find(line.p0)) {
        auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);
        auto info = m_tree.info(segment.segment_index);

        info.p0_type = SegmentPointType::input;
        m_tree.update_segment(segment.segment_index, info);
    }
    if (const auto entry = cache.logicitem_output_cache().find(line.p1)) {
        auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);
        auto info = m_tree.info(segment.segment_index);

        info.p1_type = SegmentPointType::input;
        m_tree.update_segment(segment.segment_index, info);
    }
}

auto insert_wire(State state, segment_part_t& segment_part) -> void {
    if (is_inserted(segment_part.segment.wire_id)) {
        throw_exception("segment is already inserted");
    }
    const auto target_wire_id = find_wire_for_inserting_segment(state, segment_part);

    reset_segment_endpoints(state.layout, segment_part.segment);
    discover_wire_inputs(state.layout, state.cache, segment_part.segment);
    move_segment_between_trees(state.layout, state.sender, segment_part, target_wire_id);

    const auto line = get_line(state.layout, segment_part);
    fix_and_merge_segments(state, line.p0, &segment_part);
    fix_and_merge_segments(state, line.p1, &segment_part);

    assert(is_contiguous_tree(state.layout.wires().segment_tree(target_wire_id)));
}

auto mark_valid(Layout& layout, const segment_part_t segment_part) {
    auto& m_tree = layout.wires().modifyable_segment_tree(segment_part.segment.wire_id);
    m_tree.mark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto unmark_valid(Layout& layout, const segment_part_t segment_part) {
    auto& m_tree = layout.wires().modifyable_segment_tree(segment_part.segment.wire_id);
    m_tree.unmark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto _wire_change_temporary_to_colliding(State state, segment_part_t& segment_part)
    -> void {
    const auto line = get_line(state.layout, segment_part);
    bool colliding = is_wire_colliding(state.layout, state.cache, line);

    if (colliding) {
        const auto destination = colliding_wire_id;
        move_segment_between_trees(state.layout, state.sender, segment_part, destination);
        reset_segment_endpoints(state.layout, segment_part.segment);
    } else {
        insert_wire(state, segment_part);
        mark_valid(state.layout, segment_part);
    }
}

auto _wire_change_colliding_to_insert(Layout& layout, MessageSender& sender,
                                      segment_part_t& segment_part) -> void {
    const auto wire_id = segment_part.segment.wire_id;

    // from valid
    if (is_inserted(wire_id)) {
        unmark_valid(layout, segment_part);
    }

    // from colliding
    else if (is_colliding(wire_id)) {
        remove_segment_from_tree(layout, sender, segment_part);
    }

    else {
        throw_exception("wire needs to be in inserted or colliding state");
    }
}

// we assume we get a valid tree where the part between p0 and p1
// has been removed this method puts the segments at p1 into a new tree
auto split_broken_tree(State state, point_t p0, point_t p1) -> wire_id_t {
    const auto p0_tree_id = state.cache.collision_cache().get_first_wire(p0);
    const auto p1_tree_id = state.cache.collision_cache().get_first_wire(p1);

    if (!p0_tree_id || !p1_tree_id || p0_tree_id != p1_tree_id) {
        return null_wire_id;
    };

    // create new tree
    const auto new_tree_id = add_new_wire_element(state.layout);

    // find connected segments
    const auto& tree_from = state.layout.wires().modifyable_segment_tree(p0_tree_id);
    const auto mask = calculate_connected_segments_mask(tree_from, p1);

    // move over segments
    for (const auto segment_index : tree_from.indices().reverse()) {
        if (mask[segment_index.value]) {
            auto segment_part = segment_part_t {segment_t {p0_tree_id, segment_index},
                                                tree_from.part(segment_index)};
            move_segment_between_trees(state.layout, state.sender, segment_part,
                                       new_tree_id);
        }
    }

    assert(is_contiguous_tree(tree_from));
    assert(is_contiguous_tree(state.layout.wires().segment_tree(new_tree_id)));

    return new_tree_id;
}

auto _wire_change_insert_to_colliding(Layout& layout, segment_part_t& segment_part)
    -> void {
    mark_valid(layout, segment_part);
}

auto _wire_change_colliding_to_temporary(State state, segment_part_t& segment_part)
    -> void {
    auto& layout = state.layout;

    auto source_id = segment_part.segment.wire_id;
    const auto was_inserted = is_inserted(segment_part.segment.wire_id);
    const auto moved_line = get_line(layout, segment_part);

    if (was_inserted) {
        unmark_valid(layout, segment_part);
    }

    // move to temporary
    const auto destination_id = temporary_wire_id;
    move_segment_between_trees(layout, state.sender, segment_part, destination_id);

    if (was_inserted) {
        if (layout.wires().segment_tree(source_id).empty()) {
            swap_and_delete_empty_wire(state.layout, state.sender, source_id,
                                       &segment_part.segment.wire_id);
        } else {
            fix_and_merge_segments(state, moved_line.p0);
            fix_and_merge_segments(state, moved_line.p1);

            split_broken_tree(state, moved_line.p0, moved_line.p1);
        }
        reset_segment_endpoints(state.layout, segment_part.segment);
    }
}

auto change_wire_insertion_mode_private(State state, segment_part_t& segment_part,
                                        InsertionMode new_mode) -> void {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }

    // as parts have length, the line segment can have two possible modes
    // a part could be in state valid (insert_or_discard) and another in state normal
    const auto old_modes = get_insertion_modes(state.layout, segment_part);

    if (old_modes.first == new_mode && old_modes.second == new_mode) {
        return;
    }

    if (old_modes.first == InsertionMode::temporary ||
        old_modes.second == InsertionMode::temporary) {
        _wire_change_temporary_to_colliding(state, segment_part);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _wire_change_colliding_to_insert(state.layout, state.sender, segment_part);
    }
    if (old_modes.first == InsertionMode::insert_or_discard ||
        old_modes.second == InsertionMode::insert_or_discard) {
        _wire_change_insert_to_colliding(state.layout, segment_part);
    }
    if (new_mode == InsertionMode::temporary) {
        _wire_change_colliding_to_temporary(state, segment_part);
    }
}

auto change_wire_insertion_mode(State state, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_wire_insertion_mode(segment_part = {}, new_mode = {});\n"
            "==========================================================\n\n",
            state.layout, segment_part, new_mode);
    }
    return change_wire_insertion_mode_private(state, segment_part, new_mode);
}

// adding segments

auto add_wire_segment_private(State state, ordered_line_t line,
                              InsertionMode insertion_mode) -> segment_part_t {
    auto segment_part =
        add_segment_to_tree(state.layout, state.sender, temporary_wire_id, line);

    // auto segment_part = add_segment_to_aggregate(state.layout, state.sender, line,
    //                                              display_state_t::temporary);

    change_wire_insertion_mode_private(state, segment_part, insertion_mode);

    return segment_part;
}

auto add_wire_segment(State state, ordered_line_t line, InsertionMode new_mode)
    -> segment_part_t {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_wire_segment(line = {}, new_mode = {});\n"
            "==========================================================\n\n",
            state.layout, line, new_mode);
    }
    return add_wire_segment_private(state, line, new_mode);
}

auto add_wire_segment(State state, Selection* selection, line_t line,
                      InsertionMode insertion_mode) -> void {
    auto segment_part = add_wire_segment(state, ordered_line_t {line}, insertion_mode);

    if (selection != nullptr && segment_part) {
        selection->add_segment(segment_part);
    }
}

auto add_wire_private(State state, point_t p0, point_t p1, LineInsertionType segment_type,
                      InsertionMode insertion_mode, Selection* selection) -> void {
    const auto mode = insertion_mode;

    // TODO handle p0 == p1

    switch (segment_type) {
        using enum LineInsertionType;

        case horizontal_first: {
            const auto pm = point_t {p1.x, p0.y};
            if (p0.x != pm.x) {
                add_wire_segment(state, selection, line_t {p0, pm}, mode);
            }
            if (pm.y != p1.y) {
                add_wire_segment(state, selection, line_t {pm, p1}, mode);
            }
            break;
        }

        case vertical_first: {
            const auto pm = point_t {p0.x, p1.y};
            if (p0.y != pm.y) {
                add_wire_segment(state, selection, line_t {p0, pm}, mode);
            }
            if (pm.x != p1.x) {
                add_wire_segment(state, selection, line_t {pm, p1}, mode);
            }
            break;
        }
    }
}

auto add_wire(State state, point_t p0, point_t p1, LineInsertionType segment_type,
              InsertionMode insertion_mode, Selection* selection) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_wire(p0 = {}, p1 = {}, segment_type = {}, "
            "insertion_mode = {}, *selection = {});\n"
            "==========================================================\n\n",
            state.layout, p0, p1, segment_type, insertion_mode,
            static_cast<void*>(selection));
    }
    return add_wire_private(state, p0, p1, segment_type, insertion_mode, selection);
}

auto delete_wire_segment_private(Layout& layout, MessageSender& sender,
                                 segment_part_t& segment_part) -> void {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }
    if (!is_temporary(segment_part.segment.wire_id)) [[unlikely]] {
        throw_exception("can only delete temporary segments");
    }

    remove_segment_from_tree(layout, sender, segment_part);
}

auto delete_wire_segment(Layout& layout, MessageSender& sender,
                         segment_part_t& segment_part) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "delete_wire_segment(segment_part = {});\n"
            "==========================================================\n\n",
            layout, segment_part);
    }
    return delete_wire_segment_private(layout, sender, segment_part);
}

auto is_wire_position_representable_private(const Layout& layout,
                                            const segment_part_t segment_part, int dx,
                                            int dy) -> bool {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }

    const auto line = get_line(layout, segment_part);
    return is_representable(line, dx, dy);
}

auto is_wire_position_representable(const Layout& layout,
                                    const segment_part_t segment_part, int dx, int dy)
    -> bool {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "is_wire_position_representable(segment_part = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, segment_part, dx, dy);
    }
    return is_wire_position_representable_private(layout, segment_part, dx, dy);
}

auto move_or_delete_wire_private(Layout& layout, MessageSender& sender,
                                 segment_part_t& segment_part, int dx, int dy) -> void {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }
    if (!is_temporary(segment_part.segment.wire_id)) [[unlikely]] {
        throw_exception("can only move temporary segments");
    }

    if (!is_wire_position_representable_private(layout, segment_part, dx, dy)) {
        // delete
        remove_segment_from_tree(layout, sender, segment_part);
        return;
    }

    const auto full_line = get_line(layout, segment_part.segment);
    const auto part_line = to_line(full_line, segment_part.part);

    if (full_line != part_line) {
        move_segment_between_trees(layout, sender, segment_part,
                                   segment_part.segment.wire_id);
    }

    // move
    auto& m_tree = layout.wires().modifyable_segment_tree(segment_part.segment.wire_id);
    auto info = m_tree.info(segment_part.segment.segment_index);
    info.line = add_unchecked(part_line, dx, dy);
    m_tree.update_segment(segment_part.segment.segment_index, info);

    // TODO bug missing moved messages for selection updates,
    //      maybe use a pre-build method for this?

    // messages
    if (full_line == part_line) {  // otherwise already sent in move_segment above
        sender.submit(info_message::SegmentCreated {segment_part.segment});
    }
}

auto move_or_delete_wire(Layout& layout, MessageSender& sender,
                         segment_part_t& segment_part, int dx, int dy) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_wire(segment_part = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, segment_part, dx, dy);
    }
    return move_or_delete_wire_private(layout, sender, segment_part, dx, dy);
}

auto move_wire_unchecked_private(Layout& layout, segment_t segment,
                                 part_t verify_full_part, int dx, int dy) -> void {
    // move
    auto& m_tree = layout.wires().modifyable_segment_tree(segment.wire_id);

    auto info = m_tree.info(segment.segment_index);
    info.line = add_unchecked(info.line, dx, dy);

    if (to_part(info.line) != verify_full_part) {
        throw_exception("need to select full line part");
    }

    m_tree.update_segment(segment.segment_index, info);
}

auto move_wire_unchecked(Layout& layout, segment_t segment, part_t verify_full_part,
                         int dx, int dy) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_wire(segment = {}, verify_full_part = {}, "
            "dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, segment, verify_full_part, dx, dy);
    }
    move_wire_unchecked_private(layout, segment, verify_full_part, dx, dy);
}

auto delete_all_inserted_wires(State state, point_t point) -> void {
    // segment ids change during deletion, so we need to query after each deletion
    while (true) {
        const auto segments = state.cache.spatial_cache().query_line_segments(point);

        if (!segments.at(0)) {
            return;
        }
        if (!is_inserted(segments.at(0).wire_id)) [[unlikely]] {
            throw_exception("only works on inserted elements");
        }

        const auto line = get_line(state.layout, segments.at(0));
        auto segment_part = segment_part_t {segments.at(0), to_part(line)};

        change_wire_insertion_mode_private(state, segment_part, InsertionMode::temporary);
        delete_wire_segment(state.layout, state.sender, segment_part);
    }
}

auto remove_wire_crosspoint(State state, point_t point) -> void {
    auto& layout = state.layout;

    const auto segments = state.cache.spatial_cache().query_line_segments(point);
    const auto segment_count = get_segment_count(segments);

    if (segment_count != 4) {
        return;
    }
    if (!all_same_wire_id(segments)) [[unlikely]] {
        throw_exception("expected query result to of one segment tree");
    }

    auto lines = std::array {
        get_line(layout, segments.at(0)),
        get_line(layout, segments.at(1)),
        get_line(layout, segments.at(2)),
        get_line(layout, segments.at(3)),
    };
    std::ranges::sort(lines);
    const auto new_line_0 = ordered_line_t {lines.at(0).p0, lines.at(3).p1};
    const auto new_line_1 = ordered_line_t {lines.at(1).p0, lines.at(2).p1};

    delete_all_inserted_wires(state, point);
    add_wire_segment(state, new_line_0, InsertionMode::insert_or_discard);
    add_wire_segment(state, new_line_1, InsertionMode::insert_or_discard);
}

auto add_wire_crosspoint(State state, point_t point) -> void {
    auto& layout = state.layout;

    const auto segments = state.cache.spatial_cache().query_line_segments(point);
    const auto segment_count = get_segment_count(segments);

    if (segment_count != 2) {
        return;
    }

    const auto wire_id_0 = segments.at(0).wire_id;
    const auto wire_id_1 = segments.at(1).wire_id;

    if (wire_id_0 == wire_id_1) {
        return;
    }
    if (layout.wires().segment_tree(wire_id_0).input_count() +
            layout.wires().segment_tree(wire_id_1).input_count() >
        connection_count_t {1}) {
        return;
    }

    if (!is_inserted(wire_id_0) || !is_inserted(wire_id_1)) [[unlikely]] {
        throw_exception("only works on inserted elements");
    }

    const auto line0 = get_line(layout, segments.at(0));
    const auto line1 = get_line(layout, segments.at(1));

    delete_all_inserted_wires(state, point);

    const auto mode = InsertionMode::insert_or_discard;
    add_wire_segment(state, ordered_line_t {line0.p0, point}, mode);
    add_wire_segment(state, ordered_line_t {point, line0.p1}, mode);
    add_wire_segment(state, ordered_line_t {line1.p0, point}, mode);
    add_wire_segment(state, ordered_line_t {point, line1.p1}, mode);
}

auto toggle_inserted_wire_crosspoint_private(State state, point_t point) -> void {
    if (state.cache.collision_cache().is_wires_crossing(point)) {
        return add_wire_crosspoint(state, point);
    }
    if (state.cache.collision_cache().is_wire_cross_point(point)) {
        return remove_wire_crosspoint(state, point);
    }
}

auto toggle_inserted_wire_crosspoint(State state, point_t point) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "toggle_inserted_wire_crosspoint(point = {});\n"
            "==========================================================\n\n",
            state.layout, point);
    }
    return toggle_inserted_wire_crosspoint_private(state, point);
}

//
// Handle Methods
//

auto change_insertion_mode(selection_handle_t handle, State state,
                           InsertionMode new_insertion_mode) -> void {
    if (!handle) {
        return;
    }
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= change_insertion_mode ==========\n", handle);
    }

    while (handle->selected_logic_items().size() > 0) {
        auto logicitem_id = handle->selected_logic_items()[0];
        handle->remove_logicitem(logicitem_id);

        change_logic_item_insertion_mode(state, logicitem_id, new_insertion_mode);
    }

    while (handle->selected_segments().size() > 0) {
        auto segment_part = segment_part_t {
            .segment = handle->selected_segments()[0].first,
            .part = handle->selected_segments()[0].second.front(),
        };
        handle->remove_segment(segment_part);

        change_wire_insertion_mode(state, segment_part, new_insertion_mode);
    }
}

auto new_wire_positions_representable(const Selection& selection, const Layout& layout,
                                      int delta_x, int delta_y) -> bool {
    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        for (const auto& part : parts) {
            const auto line = to_line(full_line, part);

            if (!is_representable(line, delta_x, delta_y)) {
                return false;
            }
        }
    }

    return true;
}

auto new_positions_representable(const Selection& selection, const Layout& layout,
                                 int delta_x, int delta_y) -> bool {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= new_positions_representable ==========\n", selection);
    }

    const auto logic_item_valid = [&](logicitem_id_t logicitem_id) {
        return is_logic_item_position_representable(layout, logicitem_id, delta_x,
                                                    delta_y);
    };

    return std::ranges::all_of(selection.selected_logic_items(), logic_item_valid) &&
           new_wire_positions_representable(selection, layout, delta_x, delta_y);
}

auto move_or_delete_elements(selection_handle_t handle, Layout& layout,
                             MessageSender& sender, int delta_x, int delta_y) -> void {
    if (!handle) {
        return;
    }
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= move_or_delete_elements ==========\n", handle);
    }

    while (handle->selected_logic_items().size() > 0) {
        auto logicitem_id = handle->selected_logic_items()[0];
        handle->remove_logicitem(logicitem_id);

        move_or_delete_logic_item(layout, sender, logicitem_id, delta_x, delta_y);
    }

    while (handle->selected_segments().size() > 0) {
        auto segment_part = segment_part_t {
            .segment = handle->selected_segments()[0].first,
            .part = handle->selected_segments()[0].second.front(),
        };
        handle->remove_segment(segment_part);

        move_or_delete_wire(layout, sender, segment_part, delta_x, delta_y);
    }
}

auto move_unchecked(const Selection& selection, Layout& layout, int delta_x, int delta_y)
    -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= move_unchecked ==========\n", selection);
    }

    for (const auto& logicitem_id : selection.selected_logic_items()) {
        if (layout.logic_items().display_state(logicitem_id) !=
            display_state_t::temporary) [[unlikely]] {
            throw_exception("selected logic items need to be temporary");
        }

        move_logic_item_unchecked(layout, logicitem_id, delta_x, delta_y);
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        if (parts.size() != 1) [[unlikely]] {
            throw_exception("Method assumes segments are fully selected");
        }
        if (!is_temporary(segment.wire_id)) [[unlikely]] {
            throw_exception("selected wires need to be temporary");
        }

        move_wire_unchecked(layout, segment, parts.front(), delta_x, delta_y);
    }
}

auto delete_all(selection_handle_t handle, State state) -> void {
    if (!handle) {
        return;
    }
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= delete_all ==========\n", handle);
    }

    while (handle->selected_logic_items().size() > 0) {
        auto logicitem_id = handle->selected_logic_items()[0];
        handle->remove_logicitem(logicitem_id);

        change_logic_item_insertion_mode(state, logicitem_id, InsertionMode::temporary);
        swap_and_delete_logic_item(state.layout, state.sender, logicitem_id);
    }

    while (handle->selected_segments().size() > 0) {
        auto segment_part = segment_part_t {
            .segment = handle->selected_segments()[0].first,
            .part = handle->selected_segments()[0].second.front(),
        };
        handle->remove_segment(segment_part);

        change_wire_insertion_mode(state, segment_part, InsertionMode::temporary);
        delete_wire_segment(state.layout, state.sender, segment_part);
    }
}

//
// Wire Mode Change Helpers
//

class SegmentEndpointMap {
   public:
    // orientation: right, left, up, down
    using value_t = std::array<segment_t, 4>;
    using map_t = ankerl::unordered_dense::map<point_t, value_t>;

    using mergable_t = std::pair<segment_t, segment_t>;

   public:
    [[nodiscard]] static auto index(const orientation_t orientation) {
        return static_cast<std::underlying_type<orientation_t>::type>(orientation);
    }

    [[nodiscard]] static auto get(const value_t& segments,
                                  const orientation_t orientation) {
        return segments.at(index(orientation));
    }

    [[nodiscard]] static auto has(const value_t& segments,
                                  const orientation_t orientation) {
        return get(segments, orientation) != null_segment;
    }

   private:
    [[nodiscard]] static auto count_points(const value_t& segments) {
        return std::ranges::count_if(
            segments, [](segment_t value) { return value != null_segment; });
    }

    [[nodiscard]] static auto to_adcacent_segment(const value_t& segments)
        -> std::optional<mergable_t> {
        using enum orientation_t;

        if (count_points(segments) != 2) {
            return std::nullopt;
        }

        const auto to_segment = [&](orientation_t orientation) {
            return get(segments, orientation);
        };
        const auto has_segment = [&](orientation_t orientation) {
            return has(segments, orientation);
        };

        if (has_segment(left) && has_segment(right)) {
            return mergable_t {to_segment(left), to_segment(right)};
        }

        else if (has_segment(up) && has_segment(down)) {
            return mergable_t {to_segment(up), to_segment(down)};
        }

        return std::nullopt;
    }

   public:
    auto add_segment(segment_t segment, ordered_line_t line) -> void {
        add_point(line.p0, segment, to_orientation_p0(line));
        add_point(line.p1, segment, to_orientation_p1(line));
    }

    // [](point_t point, std::array<segment_t,4> segments, int count) {}
    template <typename Func>
    auto iter_crosspoints(Func callback) const -> void {
        for (const auto& [point, segments] : map_.values()) {
            const auto count = count_points(segments);

            if (count >= 3) {
                callback(point, segments, count);
            }
        }
    }

    [[nodiscard]] auto adjacent_segments() const -> std::vector<mergable_t> {
        auto result = std::vector<mergable_t> {};

        for (const auto& [point, segments] : map_.values()) {
            if (const auto adjacent = to_adcacent_segment(segments)) {
                result.push_back(*adjacent);
            }
        }

        return result;
    }

   private:
    auto add_point(point_t point, segment_t segment, orientation_t orientation) -> void {
        const auto index = this->index(orientation);

        const auto it = map_.find(point);

        if (it != map_.end()) {
            if (it->second.at(index) != null_segment) [[unlikely]] {
                throw_exception("entry already exists in SegmentEndpointMap");
            }

            it->second.at(index) = segment;
        } else {
            auto value = value_t {null_segment, null_segment, null_segment, null_segment};
            value.at(index) = segment;

            map_.emplace(point, value);
        }
    }

   private:
    map_t map_ {};
};

auto build_endpoint_map(const Layout& layout, const Selection& selection)
    -> SegmentEndpointMap {
    auto map = SegmentEndpointMap {};

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        if (!is_temporary(segment.wire_id)) {
            throw_exception("can only merge temporary segments");
        }
        if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
            throw_exception("selection cannot contain partially selected lines");
        }

        map.add_segment(segment, full_line);
    }

    return map;
}

auto regularize_temporary_selection(Layout& layout, MessageSender& sender,
                                    const Selection& selection,
                                    std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t> {
    if (true_cross_points) {
        split_temporary_segments(layout, sender, *true_cross_points, selection);
        std::ranges::sort(*true_cross_points);
    }

    const auto map = build_endpoint_map(layout, selection);
    auto mergable_segments = map.adjacent_segments();
    auto cross_points = std::vector<point_t> {};

    map.iter_crosspoints(
        [&](point_t point, std::array<segment_t, 4> segments, int count) {
            if (count == 3 || !true_cross_points ||
                std::ranges::binary_search(*true_cross_points, point)) {
                cross_points.push_back(point);

                const auto segment = segments.at(0) ? segments.at(0) : segments.at(1);
                set_segment_crosspoint(layout, segment, point);
            } else {
                using enum orientation_t;

                mergable_segments.push_back({
                    SegmentEndpointMap::get(segments, left),
                    SegmentEndpointMap::get(segments, right),
                });
                mergable_segments.push_back({
                    SegmentEndpointMap::get(segments, up),
                    SegmentEndpointMap::get(segments, down),
                });
            }
        });

    merge_all_line_segments(layout, sender, mergable_segments);

    return cross_points;
}

auto capture_inserted_cross_points(const Layout& layout, const CacheProvider& cache,
                                   const Selection& selection) -> std::vector<point_t> {
    auto cross_points = std::vector<point_t> {};

    for (const auto& [segment, parts] : selection.selected_segments()) {
        for (const auto& part : parts) {
            const auto line = get_line(layout, segment_part_t {segment, part});

            if (cache.collision_cache().is_wire_cross_point(line.p0)) {
                cross_points.push_back(line.p0);
            }
            if (cache.collision_cache().is_wire_cross_point(line.p1)) {
                cross_points.push_back(line.p1);
            }
        }
    }

    std::ranges::sort(cross_points);
    cross_points.erase(std::ranges::unique(cross_points).begin(), cross_points.end());

    return cross_points;
}

auto split_temporary_segments(Layout& layout, MessageSender& sender,
                              std::span<const point_t> split_points,
                              const Selection& selection) -> void {
    const auto cache = SplitPointCache {split_points};
    auto query_result = std::vector<point_t> {};

    const auto segments = transform_to_vector(
        selection.selected_segments(), [&](Selection::segment_pair_t value) {
            const auto& [segment, parts] = value;

            const auto full_line = get_line(layout, segment);

            if (!is_temporary(segment.wire_id)) {
                throw_exception("can only split temporary segments");
            }
            if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
                throw_exception("selection cannot contain partially selected lines");
            }

            return segment;
        });

    for (const auto& segment : segments) {
        const auto full_line = get_line(layout, segment);

        cache.query_intersects(full_line, query_result);
        std::ranges::sort(query_result, std::greater<point_t>());
        query_result.erase(std::ranges::unique(query_result).begin(), query_result.end());

        // splitting puts the second half into a new segment
        // so for this to work with multiple point, cross_points
        // need to be sorted in descendant order
        for (const auto& point : query_result) {
            if (is_inside(point, full_line)) {
                split_line_segment(layout, sender, segment, point);
            }
        }
    }
}

auto capture_new_splitpoints(const Layout& layout, const CacheProvider& cache,
                             const Selection& selection) -> std::vector<point_t> {
    auto result = std::vector<point_t> {};

    const auto add_candidate = [&](point_t point) {
        const auto state = cache.collision_cache().query(point);
        if (collision_cache::is_wire_corner_point(state) ||
            collision_cache::is_wire_connection(state) ||
            collision_cache::is_wire_cross_point(state)) {
            result.push_back(point);
        }
    };

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        if (!is_temporary(segment.wire_id)) {
            throw_exception("can only find new split-points for temporary segments");
        }
        if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
            throw_exception("selection cannot contain partially selected lines");
        }

        if (is_horizontal(full_line)) {
            for (auto x : range(full_line.p0.x + grid_t {1}, full_line.p1.x)) {
                add_candidate(point_t {x, full_line.p0.y});
            }
        } else {
            for (auto y : range(full_line.p0.y + grid_t {1}, full_line.p1.y)) {
                add_candidate(point_t {full_line.p0.x, y});
            }
        }
    }

    return result;
}

}  // namespace editable_circuit

}  // namespace logicsim
