#include "editable_circuit/handlers.h"

#include "collision.h"
#include "editable_circuit/caches.h"
#include "editable_circuit/caches/cross_point_cache.h"
#include "editable_circuit/selection.h"
#include "editable_circuit/selection_registrar.h"
#include "format.h"
#include "geometry.h"
#include "layout_calculations.h"
#include "scene.h"
#include "timer.h"

#include <fmt/core.h>

#include <algorithm>
#include <ranges>

namespace logicsim {

namespace editable_circuit {

//
// Deletion Handling
//

auto is_wire_with_segments(const Layout& layout, const element_id_t element_id) -> bool {
    const auto element = layout.element(element_id);
    return element.is_wire() && !element.segment_tree().empty();
}

auto notify_element_deleted(const Layout& layout, MessageSender sender,
                            element_id_t element_id) {
    const auto element = layout.element(element_id);

    if (element.is_logic_item()) {
        sender.submit(info_message::LogicItemDeleted {element_id});
    }
}

auto notify_element_id_change(const Layout& layout, MessageSender sender,
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
                .segment_info = segment_tree.segment_info(segment_index),
            });
        }
    }
}

auto swap_elements(Layout& layout, MessageSender sender, const element_id_t element_id_0,
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

auto swap_and_delete_single_element_private(Layout& layout, MessageSender sender,
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

auto swap_and_delete_single_element(Layout& layout, MessageSender sender,
                                    element_id_t& element_id,
                                    element_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "swap_and_delete_single_element(element_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            layout, element_id, fmt_ptr(preserve_element));
    }
    swap_and_delete_single_element_private(layout, sender, element_id, preserve_element);
}

auto swap_and_delete_multiple_elements_private(Layout& layout, MessageSender sender,
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

auto swap_and_delete_multiple_elements(Layout& layout, MessageSender sender,
                                       std::span<const element_id_t> element_ids,
                                       element_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "swap_and_delete_multiple_elements(element_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            layout, element_ids, fmt_ptr(preserve_element));
    }
    swap_and_delete_multiple_elements_private(layout, sender, element_ids,
                                              preserve_element);
}

//
// Logic Item Handling
//

auto StandardLogicAttributes::format() const -> std::string {
    return fmt::format("{{{}, input_count = {}, {}, {}}}", type, input_count, position,
                       orientation);
}

auto is_logic_item_position_representable_private(const Layout& layout,
                                                  const element_id_t element_id, int x,
                                                  int y) -> bool {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }
    if (!is_representable(x, y)) {
        return false;
    }
    const auto position = point_t {grid_t {x}, grid_t {y}};

    auto data = to_layout_calculation_data(layout, element_id);
    data.position = position;

    return is_representable(data);
}

auto is_logic_item_position_representable(const Layout& layout,
                                          const element_id_t element_id, int x, int y)
    -> bool {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "is_logic_item_position_representable(element_id = {}, x = {}, y = {});\n"
            "==========================================================\n\n",
            layout, element_id, x, y);
    }
    return is_logic_item_position_representable_private(layout, element_id, x, y);
}

auto move_or_delete_logic_item_private(Layout& layout, MessageSender sender,
                                       element_id_t& element_id, int x, int y) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }
    if (layout.display_state(element_id) != display_state_t::temporary) [[unlikely]] {
        throw_exception("Only temporary items can be freely moded.");
    }

    if (!is_logic_item_position_representable_private(layout, element_id, x, y)) {
        swap_and_delete_single_element_private(layout, sender, element_id);
        return;
    }

    const auto position = point_t {grid_t {x}, grid_t {y}};
    layout.set_position(element_id, position);
}

auto move_or_delete_logic_item(Layout& layout, MessageSender sender,
                               element_id_t& element_id, int x, int y) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "move_or_delete_logic_item(element_id = {}, x = {}, y = {});\n"
            "==========================================================\n\n",
            layout, element_id, x, y);
    }
    move_or_delete_logic_item_private(layout, sender, element_id, x, y);
}

// mode change helpers

auto insert_logic_item(State state, element_id_t& element_id) {
    // we assume there will be no collision
    // connect_element(state, element_id);
}

// mode change

auto is_circuit_item_colliding(const Layout& layout, const CacheProvider& cache,
                               const element_id_t element_id) {
    const auto data = to_layout_calculation_data(layout, element_id);
    return cache.is_element_colliding(data);
}

auto notify_circuit_item_inserted(const Layout& layout, MessageSender sender,
                                  const element_id_t element_id) {
    const auto data = to_layout_calculation_data(layout, element_id);
    sender.submit(info_message::LogicItemInserted {element_id, data});
}

auto _element_change_temporary_to_colliding(State state, element_id_t& element_id)
    -> void {
    if (state.layout.display_state(element_id) != display_state_t::temporary)
        [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    if (is_circuit_item_colliding(state.layout, state.cache, element_id)) {
        state.layout.set_display_state(element_id, display_state_t::colliding);
    } else {
        insert_logic_item(state, element_id);
        state.layout.set_display_state(element_id, display_state_t::valid);
        notify_circuit_item_inserted(state.layout, state.sender, element_id);
    }
};

auto _element_change_colliding_to_insert(Layout& layout, MessageSender sender,
                                         element_id_t& element_id) -> void {
    const auto display_state = layout.display_state(element_id);

    if (display_state == display_state_t::valid) {
        layout.set_display_state(element_id, display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::colliding) [[likely]] {
        // we can only delete temporary elements
        layout.set_display_state(element_id, display_state_t::temporary);
        swap_and_delete_single_element_private(layout, sender, element_id);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto _element_change_insert_to_colliding(Layout& layout, const element_id_t element_id)
    -> void {
    if (layout.display_state(element_id) != display_state_t::normal) [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    layout.set_display_state(element_id, display_state_t::valid);
};

auto _element_change_colliding_to_temporary(Layout& layout, MessageSender sender,
                                            element_id_t& element_id) -> void {
    const auto display_state = layout.display_state(element_id);

    if (display_state == display_state_t::valid) {
        const auto data = to_layout_calculation_data(layout, element_id);
        sender.submit(info_message::LogicItemUninserted {element_id, data});
        layout.set_display_state(element_id, display_state_t::temporary);

        // TODO uninsert
        return;
    }

    if (display_state == display_state_t::colliding) {
        layout.set_display_state(element_id, display_state_t::temporary);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto change_logic_item_insertion_mode_private(State state, element_id_t& element_id,
                                              InsertionMode new_mode) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }
    if (!state.layout.element(element_id).is_logic_item()) [[unlikely]] {
        throw_exception("only works on logic elements");
    }

    const auto old_mode = to_insertion_mode(state.layout.display_state(element_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        _element_change_temporary_to_colliding(state, element_id);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _element_change_colliding_to_insert(state.layout, state.sender, element_id);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        _element_change_insert_to_colliding(state.layout, element_id);
    }
    if (new_mode == InsertionMode::temporary) {
        _element_change_colliding_to_temporary(state.layout, state.sender, element_id);
    }
}

auto change_logic_item_insertion_mode(State state, element_id_t& element_id,
                                      InsertionMode new_mode) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "change_logic_item_insertion_mode(element_id = {}, new_mode = {});\n"
            "==========================================================\n\n",
            state.layout, element_id, new_mode);
    }
    change_logic_item_insertion_mode_private(state, element_id, new_mode);
}

auto add_standard_logic_item_private(State state, StandardLogicAttributes attributes,
                                     InsertionMode insertion_mode) -> element_id_t {
    using enum ElementType;
    const auto type = attributes.type;

    if (!(type == and_element || type == or_element || type == xor_element
          || type == inverter_element)) [[unlikely]] {
        throw_exception("The type needs to be a standard element.");
    }
    if (type == inverter_element && attributes.input_count != 1) [[unlikely]] {
        throw_exception("Inverter needs to have exactly one input.");
    }
    if (type != inverter_element && attributes.input_count < 2) [[unlikely]] {
        throw_exception("Input count needs to be at least 2 for standard elements.");
    }

    // insert into underlyings
    auto element_id = state.layout
                          .add_element({
                              .display_state = display_state_t::temporary,
                              .element_type = attributes.type,

                              .input_count = attributes.input_count,
                              .output_count = 1,
                              .position = point_t {0, 0},
                              .orientation = attributes.orientation,
                          })
                          .element_id();
    state.sender.submit(info_message::LogicItemCreated {element_id});

    // validates our position
    move_or_delete_logic_item_private(state.layout, state.sender, element_id,  //
                                      attributes.position.x.value,             //
                                      attributes.position.y.value);
    if (element_id) {
        change_logic_item_insertion_mode_private(state, element_id, insertion_mode);
    }
    return element_id;
}

auto add_standard_logic_item(State state, StandardLogicAttributes attributes,
                             InsertionMode insertion_mode) -> element_id_t {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "add_standard_logic_item(attributes = {}, insertion_mode = {});\n"
            "==========================================================\n\n",
            state.layout, attributes, insertion_mode);
    }
    return add_standard_logic_item_private(state, attributes, insertion_mode);
}

//
// Wire Handling
//

// aggregates

auto is_wire_aggregate(const Layout& layout, const element_id_t element_id,
                       display_state_t display_state) -> bool {
    const auto element = layout.element(element_id);
    return element.is_wire() && element.display_state() == display_state;
}

auto add_new_wire_element(Layout& layout, display_state_t display_state) -> element_id_t {
    return layout
        .add_element(Layout::ElementData {
            .display_state = display_state,
            .element_type = ElementType::wire,

            .input_count = 0,
            .output_count = 0,
        })
        .element_id();
}

auto find_wire(const Layout& layout, display_state_t display_state) -> element_id_t {
    const auto element_ids = layout.element_ids();
    const auto it
        = std::ranges::find_if(element_ids, [&](element_id_t element_id) -> bool {
              return is_wire_aggregate(layout, element_id, display_state);
          });
    return it == element_ids.end() ? null_element : *it;
}

auto create_aggregate_tree_at(Layout& layout, MessageSender sender,
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

auto create_aggregate_wires(Layout& layout, MessageSender sender) -> void {
    using enum display_state_t;
    create_aggregate_tree_at(layout, sender, temporary, TEMPORARY_AGGREGATE_ID);
    create_aggregate_tree_at(layout, sender, colliding, COLLIDING_AGGREGATE_ID);
}

auto get_or_create_aggregate(Layout& layout, MessageSender sender,
                             display_state_t display_state) -> element_id_t {
    using enum display_state_t;

    // temporary
    if (display_state == temporary) {
        if (layout.element_count() <= TEMPORARY_AGGREGATE_ID.value
            || !is_wire_aggregate(layout, TEMPORARY_AGGREGATE_ID, temporary)) {
            create_aggregate_wires(layout, sender);
        }
        return TEMPORARY_AGGREGATE_ID;
    }

    // colliding
    else if (display_state == colliding) {
        if (layout.element_count() <= COLLIDING_AGGREGATE_ID.value
            || !is_wire_aggregate(layout, COLLIDING_AGGREGATE_ID, temporary)) {
            create_aggregate_wires(layout, sender);
        }
        return COLLIDING_AGGREGATE_ID;
    }

    throw_exception("display state has no aggregate");
}

auto add_segment_to_tree(Layout& layout, MessageSender sender,
                         const element_id_t element_id, ordered_line_t line)
    -> segment_part_t {
    // insert new segment
    auto& m_tree = layout.modifyable_segment_tree(element_id);

    const auto segment_info = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };
    const auto segment_index = m_tree.add_segment(segment_info);
    const auto segment = segment_t {element_id, segment_index};

    // messages
    sender.submit(info_message::SegmentCreated {segment});
    if (is_inserted(layout, element_id)) {
        sender.submit(info_message::SegmentInserted {segment, segment_info});
    }

    return segment_part_t {segment, to_part(line)};
}

auto reset_segment_endpoints(Layout& layout, const segment_t segment) {
    if (is_inserted(layout, segment.element_id)) [[unlikely]] {
        throw_exception("cannot reset endpoints of inserted wire segment");
    }
    auto& m_tree = layout.modifyable_segment_tree(segment.element_id);

    const auto new_info = segment_info_t {
        .line = m_tree.segment_line(segment.segment_index),
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };

    m_tree.update_segment(segment.segment_index, new_info);
}

auto set_segment_crosspoint(Layout& layout, const segment_t segment, point_t point) {
    if (is_inserted(layout, segment.element_id)) [[unlikely]] {
        throw_exception("cannot set endpoints of inserted wire segment");
    }
    auto& m_tree = layout.modifyable_segment_tree(segment.element_id);

    auto info = m_tree.segment_info(segment.segment_index);

    if (info.line.p0 == point) {
        info.p0_type = SegmentPointType::cross_point;
    } else if (info.line.p1 == point) {
        info.p1_type = SegmentPointType::cross_point;
    } else [[unlikely]] {
        throw_exception("point is not part of line.");
    }

    m_tree.update_segment(segment.segment_index, info);
}

auto add_segment_to_aggregate(Layout& layout, MessageSender sender,
                              const ordered_line_t line,
                              const display_state_t aggregate_type) -> segment_part_t {
    const auto element_id = get_or_create_aggregate(layout, sender, aggregate_type);
    return add_segment_to_tree(layout, sender, element_id, line);
}

// insertion mode changing

auto is_wire_colliding(const CacheProvider& cache, const ordered_line_t line) -> bool {
    // TODO connections colliding
    return cache.collision_cache().is_colliding(line);
}

auto get_display_states(const Layout& layout, const segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t> {
    using enum display_state_t;

    const auto& tree = layout.segment_tree(segment_part.segment.element_id);
    const auto tree_state = layout.display_state(segment_part.segment.element_id);

    // aggregates
    if (tree_state == temporary || tree_state == colliding) {
        return std::make_pair(tree_state, tree_state);
    }

    // check valid parts
    for (const auto valid_part : tree.valid_parts(segment_part.segment.segment_index)) {
        // parts can not touch or overlapp, so we can return early
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
auto notify_segment_insertion_status_changed(Layout& layout, MessageSender sender,
                                             const segment_t source_segment,
                                             const segment_t destination_segment,
                                             const segment_t last_segment) {
    const auto source_inserted = is_inserted(layout, source_segment.element_id);
    const auto destination_inserted = is_inserted(layout, destination_segment.element_id);

    const auto info = get_segment_info(layout, destination_segment);

    // insertion / uninsertion
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
auto notify_segment_id_changed(MessageSender sender, const segment_t source_segment,
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

auto _move_full_segment_between_trees(Layout& layout, MessageSender sender,
                                      segment_t& source_segment,
                                      const element_id_t destination_element_id) {
    if (source_segment.element_id == destination_element_id) {
        return;
    }
    const auto source_index = source_segment.segment_index;

    auto& m_tree_source = layout.modifyable_segment_tree(source_segment.element_id);
    auto& m_tree_destination = layout.modifyable_segment_tree(destination_element_id);

    // copy
    const auto destination_index
        = m_tree_destination.copy_segment(m_tree_source, source_index);
    const auto last_index = m_tree_source.last_index();
    m_tree_source.swap_and_delete_segment(source_index);

    // messages
    const auto destination_segment
        = segment_t {destination_element_id, destination_index};
    const auto last_segment = segment_t {source_segment.element_id, last_index};

    notify_segment_id_changed(sender, source_segment, destination_segment, last_segment);
    notify_segment_insertion_status_changed(layout, sender, source_segment,
                                            destination_segment, last_segment);

    source_segment = destination_segment;
}

auto copy_segment(Layout& layout, MessageSender sender,
                  const segment_part_t source_segment_part,
                  const element_id_t destination_element_id) -> segment_part_t {
    auto& m_tree_source
        = layout.modifyable_segment_tree(source_segment_part.segment.element_id);
    auto& m_tree_destination = layout.modifyable_segment_tree(destination_element_id);

    const auto destination_index = m_tree_destination.copy_segment(
        m_tree_source, source_segment_part.segment.segment_index,
        source_segment_part.part);

    const auto destination_segment_part
        = segment_part_t {segment_t {destination_element_id, destination_index},
                          m_tree_destination.segment_part(destination_index)};

    sender.submit(info_message::SegmentCreated {destination_segment_part.segment});

    if (is_inserted(layout, destination_element_id)) {
        sender.submit(info_message::SegmentInserted({
            .segment = destination_segment_part.segment,
            .segment_info = get_segment_info(layout, destination_segment_part.segment),
        }));
    }

    return destination_segment_part;
}

auto shrink_segment_begin(Layout& layout, MessageSender sender, const segment_t segment)
    -> void {
    using namespace info_message;

    if (is_inserted(layout, segment.element_id)) {
        auto& m_tree = layout.modifyable_segment_tree(segment.element_id);
        const auto old_info = m_tree.segment_info(segment.segment_index);
        sender.submit(SegmentUninserted({.segment = segment, .segment_info = old_info}));
    }
}

auto shrink_segment_end(Layout& layout, MessageSender sender, const segment_t segment,
                        const part_t part_kept) -> segment_part_t {
    using namespace info_message;
    auto& m_tree = layout.modifyable_segment_tree(segment.element_id);
    m_tree.shrink_segment(segment.segment_index, part_kept);

    if (is_inserted(layout, segment.element_id)) {
        const auto new_info = m_tree.segment_info(segment.segment_index);
        sender.submit(SegmentInserted({.segment = segment, .segment_info = new_info}));
    }

    return segment_part_t {
        .segment = segment,
        .part = m_tree.segment_part(segment.segment_index),
    };
}

auto _move_touching_segment_between_trees(Layout& layout, MessageSender sender,
                                          segment_part_t& source_segment_part,
                                          const element_id_t destination_element_id) {
    const auto full_part = to_part(get_line(layout, source_segment_part.segment));
    const auto part_kept
        = difference_touching_one_side(full_part, source_segment_part.part);

    // move
    shrink_segment_begin(layout, sender, source_segment_part.segment);
    const auto destination_segment_part
        = copy_segment(layout, sender, source_segment_part, destination_element_id);
    const auto leftover_segment_part
        = shrink_segment_end(layout, sender, source_segment_part.segment, part_kept);

    // messages
    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination = destination_segment_part,
        .segment_part_source = source_segment_part,
    });

    if (part_kept.begin != full_part.begin) {
        sender.submit(info_message::SegmentPartMoved {
            .segment_part_destination = leftover_segment_part,
            .segment_part_source
            = segment_part_t {.segment = source_segment_part.segment, .part = part_kept},
        });
    }

    source_segment_part = destination_segment_part;
}

auto _move_splitting_segment_between_trees(Layout& layout, MessageSender sender,
                                           segment_part_t& source_segment_part,
                                           const element_id_t destination_element_id) {
    const auto full_part = to_part(get_line(layout, source_segment_part.segment));
    const auto [part0, part1]
        = difference_not_touching(full_part, source_segment_part.part);

    // move
    const auto source_part1 = segment_part_t {source_segment_part.segment, part1};

    shrink_segment_begin(layout, sender, source_segment_part.segment);
    const auto destination_part1
        = copy_segment(layout, sender, source_part1, source_part1.segment.element_id);
    const auto destination_segment_part
        = copy_segment(layout, sender, source_segment_part, destination_element_id);
    shrink_segment_end(layout, sender, source_segment_part.segment, part0);

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
auto move_segment_between_trees(Layout& layout, MessageSender sender,
                                segment_part_t& segment_part,
                                const element_id_t destination_element_id) -> void {
    const auto moving_part = segment_part.part;
    const auto full_line = get_line(layout, segment_part.segment);
    const auto full_part = to_part(full_line);

    if (a_equal_b(moving_part, full_part)) {
        _move_full_segment_between_trees(layout, sender, segment_part.segment,
                                         destination_element_id);
    } else if (a_inside_b_touching_one_side(moving_part, full_part)) {
        _move_touching_segment_between_trees(layout, sender, segment_part,
                                             destination_element_id);
    } else if (a_inside_b_not_touching(moving_part, full_part)) {
        _move_splitting_segment_between_trees(layout, sender, segment_part,
                                              destination_element_id);
    } else {
        throw_exception("segment part is invalid");
    }
}

auto _remove_full_segment_from_tree(Layout& layout, MessageSender sender,
                                    segment_part_t& full_segment_part) {
    const auto element_id = full_segment_part.segment.element_id;
    const auto segment_index = full_segment_part.segment.segment_index;
    auto& m_tree = layout.modifyable_segment_tree(element_id);

    // delete
    const auto last_index = m_tree.last_index();
    m_tree.swap_and_delete_segment(segment_index);

    // messages
    sender.submit(info_message::SegmentPartDeleted {full_segment_part});

    if (last_index != segment_index) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_t {element_id, segment_index},
            .old_segment = segment_t {element_id, last_index},
        });
    }

    full_segment_part = null_segment_part;
}

auto _remove_touching_segment_from_tree(Layout& layout, MessageSender sender,
                                        segment_part_t& segment_part) {
    const auto element_id = segment_part.segment.element_id;
    const auto index = segment_part.segment.segment_index;
    const auto part = segment_part.part;

    auto& m_tree = layout.modifyable_segment_tree(element_id);

    const auto full_part = m_tree.segment_part(index);
    const auto part_kept = difference_touching_one_side(full_part, part);

    // delete
    m_tree.shrink_segment(index, part_kept);

    // messages
    sender.submit(info_message::SegmentPartDeleted {segment_part});

    if (part_kept.begin != full_part.begin) {
        sender.submit(info_message::SegmentPartMoved {
            .segment_part_destination
            = segment_part_t {.segment = segment_part.segment,
                              .part = m_tree.segment_part(index)},
            .segment_part_source
            = segment_part_t {.segment = segment_part.segment, .part = part_kept},
        });
    }

    segment_part = null_segment_part;
}

auto _remove_splitting_segment_from_tree(Layout& layout, MessageSender sender,
                                         segment_part_t& segment_part) {
    const auto element_id = segment_part.segment.element_id;
    const auto index = segment_part.segment.segment_index;
    const auto part = segment_part.part;

    auto& m_tree = layout.modifyable_segment_tree(segment_part.segment.element_id);

    const auto full_part = m_tree.segment_part(index);
    const auto [part0, part1] = difference_not_touching(full_part, part);

    // delete
    const auto index1 = m_tree.copy_segment(m_tree, index, part1);
    m_tree.shrink_segment(index, part0);

    // messages
    const auto segment_part_1
        = segment_part_t {segment_t {element_id, index1}, m_tree.segment_part(index1)};

    sender.submit(info_message::SegmentCreated {segment_part_1.segment});

    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination = segment_part_1,
        .segment_part_source = segment_part_t {segment_part.segment, part1}});

    sender.submit(info_message::SegmentPartDeleted {segment_part});

    segment_part = null_segment_part;
}

//  * trees can become empty
//  * inserts new endpoints as shaddow points
//  * will not send insert / uninsert messages
auto remove_segment_from_tree(Layout& layout, MessageSender sender,
                              segment_part_t& segment_part) -> void {
    if (is_inserted(layout, segment_part.segment.element_id)) [[unlikely]] {
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

auto merge_and_delete_tree(Layout& layout, MessageSender sender,
                           element_id_t& tree_destination, element_id_t& tree_source)
    -> void {
    if (tree_destination >= tree_source) [[unlikely]] {
        // optimization
        throw_exception("source is deleted and should have larget id");
    }

    if (!is_inserted(layout, tree_source) && !is_inserted(layout, tree_destination))
        [[unlikely]] {
        throw_exception("only supports merging of inserted trees");
    }

    auto& m_tree_source = layout.modifyable_segment_tree(tree_source);
    auto& m_tree_destination = layout.modifyable_segment_tree(tree_destination);

    auto new_index = m_tree_destination.last_index();

    for (auto old_index : m_tree_source.indices()) {
        const auto segment_info = m_tree_source.segment_info(old_index);
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
    layout.set_display_state(tree_source, display_state_t::temporary);
    swap_and_delete_single_element_private(layout, sender, tree_source,
                                           &tree_destination);
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

using point_update_t
    = std::initializer_list<const std::pair<segment_index_t, SegmentPointType>>;

auto update_segment_point_types(Layout& layout, MessageSender sender,
                                element_id_t element_id, point_update_t data,
                                const point_t position) -> void {
    if (data.size() == 0) {
        return;
    }
    if (!is_inserted(layout, element_id)) [[unlikely]] {
        throw_exception("only works for inserted segment trees.");
    }
    auto& m_tree = layout.modifyable_segment_tree(element_id);

    const auto run_point_update = [&](bool set_to_shadow) {
        for (auto [segment_index, point_type] : data) {
            const auto old_info = m_tree.segment_info(segment_index);
            const auto new_info = updated_segment_info(
                old_info, position,
                set_to_shadow ? SegmentPointType::shadow_point : point_type);

            if (old_info != new_info) {
                m_tree.update_segment(segment_index, new_info);

                sender.submit(info_message::InsertedEndPointsUpdated {
                    .segment = segment_t {element_id, segment_index},
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

auto _merge_line_segments_ordered(Layout& layout, MessageSender sender,
                                  const segment_t segment_0, const segment_t segment_1,
                                  segment_part_t* preserve_segment) -> void {
    if (segment_0.element_id != segment_1.element_id) [[unlikely]] {
        throw_exception("Cannot merge segments of different trees.");
    }
    if (!is_inserted(layout, segment_0.element_id)) [[unlikely]] {
        throw_exception("Can only merge inserted segments.");
    }
    if (segment_0.segment_index >= segment_1.segment_index) [[unlikely]] {
        throw_exception("Segment indices need to be ordered and not the same.");
    }

    const auto index_0 = segment_0.segment_index;
    const auto index_1 = segment_1.segment_index;
    const auto element_id = segment_0.element_id;

    auto& m_tree = layout.modifyable_segment_tree(element_id);
    const auto index_last = m_tree.last_index();
    const auto segment_last = segment_t {element_id, index_last};

    const auto info_0 = m_tree.segment_info(index_0);
    const auto info_1 = m_tree.segment_info(index_1);

    // merge
    m_tree.swap_and_merge_segment(index_0, index_1);
    const auto info_merged = m_tree.segment_info(index_0);

    // messages
    sender.submit(info_message::SegmentUninserted {segment_0, info_0});
    sender.submit(info_message::SegmentUninserted {segment_1, info_1});
    sender.submit(info_message::SegmentInserted {segment_0, info_merged});

    if (to_part(info_0.line) != to_part(info_merged.line, info_0.line)) {
        sender.submit(info_message::SegmentPartMoved {
            .segment_part_destination
            = segment_part_t {segment_0, to_part(info_merged.line, info_0.line)},
            .segment_part_source = segment_part_t {segment_0, to_part(info_0.line)},
        });
    }

    sender.submit(info_message::SegmentPartMoved {
        .segment_part_destination
        = segment_part_t {segment_0, to_part(info_merged.line, info_1.line)},
        .segment_part_source = segment_part_t {segment_1, to_part(info_1.line)},
    });

    if (index_1 != index_last) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
        });
        sender.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
            .segment_info = m_tree.segment_info(index_1),
        });
    }

    // preserve
    if (preserve_segment && preserve_segment->segment.element_id == element_id) {
        const auto p_index = preserve_segment->segment.segment_index;

        if (p_index == index_0 || p_index == index_1) {
            const auto p_info = p_index == index_0 ? info_0 : info_1;
            const auto p_line = to_line(p_info.line, preserve_segment->part);
            const auto p_part = to_part(info_merged.line, p_line);
            *preserve_segment = segment_part_t {segment_t {element_id, index_0}, p_part};
        }

        else if (p_index == index_last) {
            const auto p_part = preserve_segment->part;
            *preserve_segment = segment_part_t {segment_t {element_id, index_1}, p_part};
        }
    }
}

auto merge_line_segments(Layout& layout, MessageSender sender, segment_t segment_0,
                         segment_t segment_1, segment_part_t* preserve_segment) -> void {
    if (segment_0.segment_index < segment_1.segment_index) {
        _merge_line_segments_ordered(layout, sender, segment_0, segment_1,
                                     preserve_segment);
    } else {
        _merge_line_segments_ordered(layout, sender, segment_1, segment_0,
                                     preserve_segment);
    }
}

auto split_line_segment(Layout& layout, MessageSender sender, const segment_t segment,
                        const point_t position) -> segment_part_t {
    const auto full_line = get_line(layout, segment);
    const auto line_moved = ordered_line_t {position, full_line.p1};

    auto move_segment_part = segment_part_t {segment, to_part(full_line, line_moved)};
    move_segment_between_trees(layout, sender, move_segment_part, segment.element_id);

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
    const auto element_id = get_unique_element_id(segments);
    const auto indices = get_segment_indices(segments);

    if (segment_count == 1) {
        update_segment_point_types(
            state.layout, state.sender, element_id,
            {
                std::pair {indices.at(0), SegmentPointType::output},
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
                               segment_t {element_id, lines.at(0).second}, position);
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
            state.layout, state.sender, element_id,
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
                state.layout, state.sender, element_id,
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
            state.layout, state.sender, element_id,
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
    -> element_id_t {
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
        if (segment_part.segment.element_id > candidate_0
            || segment_part.segment.element_id > candidate_1) {
            throw_exception("cannot preserve segment element_id");
        }

        if (candidate_0 > candidate_1) {
            using std::swap;
            swap(candidate_0, candidate_1);
        }

        merge_and_delete_tree(state.layout, state.sender, candidate_0, candidate_1);
        return candidate_0;
    }

    // 0 wires
    return add_new_wire_element(state.layout, display_state_t::normal);
}

auto insert_wire(State state, segment_part_t& segment_part) -> void {
    if (is_inserted(state.layout, segment_part.segment.element_id)) {
        throw_exception("segment is already inserted");
    }
    const auto target_wire_id = find_wire_for_inserting_segment(state, segment_part);

    reset_segment_endpoints(state.layout, segment_part.segment);
    move_segment_between_trees(state.layout, state.sender, segment_part, target_wire_id);

    const auto line = get_line(state.layout, segment_part);
    fix_and_merge_segments(state, line.p0, &segment_part);
    fix_and_merge_segments(state, line.p1, &segment_part);

    // TODO connect segment

#ifndef NDEBUG
    state.layout.segment_tree(target_wire_id).validate_inserted();
#endif
}

auto mark_valid(Layout& layout, const segment_part_t segment_part) {
    auto& m_tree = layout.modifyable_segment_tree(segment_part.segment.element_id);
    m_tree.mark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto unmark_valid(Layout& layout, const segment_part_t segment_part) {
    auto& m_tree = layout.modifyable_segment_tree(segment_part.segment.element_id);
    m_tree.unmark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto _wire_change_temporary_to_colliding(State state, segment_part_t& segment_part)
    -> void {
    const auto line = get_line(state.layout, segment_part);
    bool colliding = is_wire_colliding(state.cache, line);

    if (colliding) {
        const auto destination = get_or_create_aggregate(state.layout, state.sender,
                                                         display_state_t::colliding);
        move_segment_between_trees(state.layout, state.sender, segment_part, destination);
    } else {
        insert_wire(state, segment_part);
        mark_valid(state.layout, segment_part);
    }
}

auto _wire_change_colliding_to_insert(Layout& layout, MessageSender sender,
                                      segment_part_t& segment_part) -> void {
    using enum display_state_t;
    const auto element_id = segment_part.segment.element_id;
    const auto display_state = layout.display_state(element_id);

    // from valid
    if (display_state == normal || display_state == valid) {
        auto& m_tree = layout.modifyable_segment_tree(element_id);
        m_tree.unmark_valid(segment_part.segment.segment_index, segment_part.part);
    }

    // from colliding
    else if (display_state == colliding) {
        remove_segment_from_tree(layout, sender, segment_part);
    }

    else {
        throw_exception("wire needs to be in inserted or colliding state");
    }
}

auto delete_empty_tree(Layout& layout, MessageSender sender, element_id_t element_id,
                       element_id_t* preserve_element = nullptr) {
    if (!is_inserted(layout, element_id) || !layout.segment_tree(element_id).empty()) {
        throw_exception("can only delete empty inserted segment trees");
    }

    layout.set_display_state(element_id, display_state_t::temporary);
    swap_and_delete_single_element_private(layout, sender, element_id, preserve_element);
}

// we assume we get a valid tree where the part between p0 and p1 has been removed
// this method puts the segments at p1 into a new tree
auto split_broken_tree(State state, point_t p0, point_t p1) -> element_id_t {
    const auto p0_tree_id = state.cache.collision_cache().get_first_wire(p0);
    const auto p1_tree_id = state.cache.collision_cache().get_first_wire(p1);

    if (!p0_tree_id || !p1_tree_id || p0_tree_id != p1_tree_id) {
        return null_element;
    };

    // create new tree
    const auto display_state = state.layout.display_state(p0_tree_id);
    const auto new_tree_id = add_new_wire_element(state.layout, display_state);

    // find connected segments
    const auto& tree_from = state.layout.modifyable_segment_tree(p0_tree_id);
    const auto mask = calculate_connected_segments_mask(tree_from, p1);

    // move over segments
    for (const auto segment_index : tree_from.indices().reverse()) {
        if (mask[segment_index.value]) {
            auto segment_part = segment_part_t {segment_t {p0_tree_id, segment_index},
                                                tree_from.segment_part(segment_index)};
            move_segment_between_trees(state.layout, state.sender, segment_part,
                                       new_tree_id);
        }
    }

#ifndef NDEBUG
    tree_from.validate_inserted();
    state.layout.segment_tree(new_tree_id).validate_inserted();
#endif

    return new_tree_id;
}

auto _wire_change_insert_to_colliding(Layout& layout, segment_part_t& segment_part)
    -> void {
    mark_valid(layout, segment_part);
}

auto _wire_change_colliding_to_temporary(State state, segment_part_t& segment_part)
    -> void {
    auto& layout = state.layout;

    const auto source_id = segment_part.segment.element_id;
    const auto was_inserted = is_inserted(layout, segment_part.segment.element_id);
    const auto moved_line = get_line(layout, segment_part);

    if (was_inserted) {
        unmark_valid(layout, segment_part);
    }

    // move to temporary
    const auto destination_id
        = get_or_create_aggregate(state.layout, state.sender, display_state_t::temporary);
    move_segment_between_trees(layout, state.sender, segment_part, destination_id);

    if (was_inserted) {
        if (layout.segment_tree(source_id).empty()) {
            delete_empty_tree(state.layout, state.sender, source_id,
                              &segment_part.segment.element_id);
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
    if (!state.layout.element(segment_part.segment.element_id).is_wire()) [[unlikely]] {
        throw_exception("only works for wires");
    }

    // as parts have length, the line segment can have two possible modes
    // a part could be in state valid (insert_or_discard) and another in state normal
    const auto old_modes = get_insertion_modes(state.layout, segment_part);

    if (old_modes.first == new_mode && old_modes.second == new_mode) {
        return;
    }

    if (old_modes.first == InsertionMode::temporary
        || old_modes.second == InsertionMode::temporary) {
        _wire_change_temporary_to_colliding(state, segment_part);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _wire_change_colliding_to_insert(state.layout, state.sender, segment_part);
    }
    if (old_modes.first == InsertionMode::insert_or_discard
        || old_modes.second == InsertionMode::insert_or_discard) {
        _wire_change_insert_to_colliding(state.layout, segment_part);
    }
    if (new_mode == InsertionMode::temporary) {
        _wire_change_colliding_to_temporary(state, segment_part);
    }
}

auto change_wire_insertion_mode(State state, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
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
    auto segment_part = add_segment_to_aggregate(state.layout, state.sender, line,
                                                 display_state_t::temporary);

    change_wire_insertion_mode_private(state, segment_part, insertion_mode);

    return segment_part;
}

auto add_wire_segment(State state, ordered_line_t line, InsertionMode new_mode)
    -> segment_part_t {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
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

auto add_wire_private(State state, point_t p0, point_t p1, LineSegmentType segment_type,
                      InsertionMode insertion_mode, Selection* selection) -> void {
    const auto mode = insertion_mode;

    // TODO handle p0 == p1

    switch (segment_type) {
        using enum LineSegmentType;

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

auto add_wire(State state, point_t p0, point_t p1, LineSegmentType segment_type,
              InsertionMode insertion_mode, Selection* selection) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "add_wire(p0 = {}, p1 = {}, segment_type = {}, "
            "insertion_mode = {}, *selection = {});\n"
            "==========================================================\n\n",
            state.layout, p0, p1, segment_type, insertion_mode,
            static_cast<void*>(selection));
    }
    return add_wire_private(state, p0, p1, segment_type, insertion_mode, selection);
}

auto delete_wire_segment_private(Layout& layout, MessageSender sender,
                                 segment_part_t& segment_part) -> void {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }
    if (layout.display_state(segment_part.segment.element_id)
        != display_state_t::temporary) [[unlikely]] {
        throw_exception("can only delete temporary segments");
    }

    remove_segment_from_tree(layout, sender, segment_part);
}

auto delete_wire_segment(Layout& layout, MessageSender sender,
                         segment_part_t& segment_part) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
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
        fmt::print(
            "\n==========================================================\n{}\n"
            "is_wire_position_representable(segment_part = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, segment_part, dx, dy);
    }
    return is_wire_position_representable_private(layout, segment_part, dx, dy);
}

auto move_or_delete_wire_private(Layout& layout, MessageSender sender,
                                 segment_part_t& segment_part, int dx, int dy) -> void {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }
    if (layout.display_state(segment_part.segment.element_id)
        != display_state_t::temporary) [[unlikely]] {
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
                                   segment_part.segment.element_id);
    }

    // move
    auto& m_tree = layout.modifyable_segment_tree(segment_part.segment.element_id);
    auto info = m_tree.segment_info(segment_part.segment.segment_index);
    info.line = add_unchecked(part_line, dx, dy);
    m_tree.update_segment(segment_part.segment.segment_index, info);

    // messages
    if (full_line == part_line) {  // otherwise already sent in move_segment above
        sender.submit(info_message::SegmentCreated {segment_part.segment});
    }
}

auto move_or_delete_wire(Layout& layout, MessageSender sender,
                         segment_part_t& segment_part, int dx, int dy) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "move_or_delete_wire(segment_part = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            layout, segment_part, dx, dy);
    }
    return move_or_delete_wire_private(layout, sender, segment_part, dx, dy);
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
        auto element_id = handle->selected_logic_items()[0];
        handle->remove_logicitem(element_id);

        change_logic_item_insertion_mode(state, element_id, new_insertion_mode);
    }

    // when we remove segments of cross points, the other segments might be
    // merged. We store those points, so we later split them again when
    // they are moved into the temporary aggregate
    auto cross_points = CrossPointCache {};
    auto query_result = std::vector<point_t> {};

    while (handle->selected_segments().size() > 0) {
        auto segment_part = segment_part_t {
            .segment = handle->selected_segments()[0].first,
            .part = handle->selected_segments()[0].second.at(0),
        };
        handle->remove_segment(segment_part);

        auto p0 = std::optional<point_t> {};
        auto p1 = std::optional<point_t> {};

        bool uninserted = new_insertion_mode == InsertionMode::temporary
                          && is_inserted(state.layout, segment_part.segment.element_id);
        if (uninserted) {
            const auto line = get_line(state.layout, segment_part);

            if (state.cache.collision_cache().is_wire_cross_point(line.p0)) {
                p0 = line.p0;
            }
            if (state.cache.collision_cache().is_wire_cross_point(line.p1)) {
                p1 = line.p1;
            }
        }

        change_wire_insertion_mode(state, segment_part, new_insertion_mode);

        if (uninserted) {
            const auto segment = segment_part.segment;
            const auto line = get_line(state.layout, segment);

            cross_points.query_intersects(line, query_result);
            std::ranges::sort(query_result, std::greater<point_t>());
            query_result.erase(std::ranges::unique(query_result).begin(),
                               query_result.end());

            // splitting puts the second half into a new segment
            // so for this to work with multiple point, cross_points
            // need to be sorted in descendant order
            for (auto point : query_result) {
                if (is_inside(point, line)) {
                    split_line_segment(state.layout, state.sender, segment, point);
                }
                set_segment_crosspoint(state.layout, segment, point);
            }
        }

        if (p0.has_value()) {
            cross_points.add_cross_point(p0.value());
        }
        if (p1.has_value()) {
            cross_points.add_cross_point(p1.value());
        }
    }
}

auto position_calculator(const Layout& layout, int delta_x, int delta_y) {
    return [delta_x, delta_y, &layout](element_id_t element_id) {
        const auto& element_position = layout.position(element_id);

        const int x = element_position.x.value + delta_x;
        const int y = element_position.y.value + delta_y;

        return std::make_pair(x, y);
    };
};

auto new_positions_representable(const Selection& selection, const Layout& layout,
                                 int delta_x, int delta_y) -> bool {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= new_positions_representable ==========\n", selection);
    }

    const auto get_position = position_calculator(layout, delta_x, delta_y);

    const auto is_valid = [&](element_id_t element_id) {
        const auto [x, y] = get_position(element_id);
        return is_logic_item_position_representable(layout, element_id, x, y);
    };
    return std::ranges::all_of(selection.selected_logic_items(), is_valid);
}

auto move_or_delete_elements(selection_handle_t handle, Layout& layout,
                             MessageSender sender, int delta_x, int delta_y) -> void {
    if (!handle) {
        return;
    }
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        print("\n\n========= move_or_delete_elements ==========\n", handle);
    }

    const auto get_position = position_calculator(layout, delta_x, delta_y);

    while (handle->selected_logic_items().size() > 0) {
        auto element_id = handle->selected_logic_items()[0];
        handle->remove_logicitem(element_id);

        const auto [x, y] = get_position(element_id);
        move_or_delete_logic_item(layout, sender, element_id, x, y);
    }

    while (handle->selected_segments().size() > 0) {
        auto segment_part = segment_part_t {
            .segment = handle->selected_segments()[0].first,
            .part = handle->selected_segments()[0].second.at(0),
        };
        handle->remove_segment(segment_part);

        move_or_delete_wire(layout, sender, segment_part, delta_x, delta_y);
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
        auto element_id = handle->selected_logic_items()[0];
        handle->remove_logicitem(element_id);

        change_logic_item_insertion_mode(state, element_id, InsertionMode::temporary);
        swap_and_delete_single_element(state.layout, state.sender, element_id);
    }

    while (handle->selected_segments().size() > 0) {
        auto segment_part = segment_part_t {
            .segment = handle->selected_segments()[0].first,
            .part = handle->selected_segments()[0].second.at(0),
        };
        handle->remove_segment(segment_part);

        change_wire_insertion_mode(state, segment_part, InsertionMode::temporary);
        delete_wire_segment(state.layout, state.sender, segment_part);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
