#include "editable_circuit/handlers.h"

#include "editable_circuit/caches.h"
#include "layout_calculations.h"
#include "scene.h"
#include "selection.h"

namespace logicsim {

namespace editable_circuit {

//
// Deletion Handler
//

auto swap_and_delete_multiple_elements(Circuit& circuit, MessageSender sender,
                                       std::span<const element_id_t> element_ids)
    -> void {
    // sort descending, so we don't invalidate our ids
    auto sorted_ids = delete_queue_t {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (auto element_id : sorted_ids) {
        swap_and_delete_single_element(circuit, sender, element_id);
    }
}

auto notify_element_deleted() {}

auto notify_element_id_change(const Circuit& circuit, MessageSender sender,
                              element_id_t new_element_id, element_id_t old_element_id) {
    const auto& layout = circuit.layout();
    const auto& schematic = circuit.schematic();
    const auto element = schematic.element(new_element_id);

    if (element.is_placeholder()) {
        return;
    }

    sender.submit(info_message::ElementUpdated {
        .new_element_id = new_element_id,
        .old_element_id = old_element_id,
    });

    bool inserted = is_inserted(circuit, new_element_id);

    if (inserted && element.is_wire()) {
        const auto& segment_tree = layout.segment_tree(new_element_id);

        for (auto&& segment_index : segment_tree.indices()) {
            sender.submit(info_message::InsertedSegmentUpdated {
                .new_segment = segment_t {new_element_id, segment_index},
                .old_segment = segment_t {old_element_id, segment_index},
                .segment_info = segment_tree.segment(segment_index),
            });
        }
    }

    if (inserted && element.is_logic_item()) {
        const auto data = to_layout_calculation_data(circuit, new_element_id);

        sender.submit(info_message::InsertedLogicItemUpdated {
            .new_element_id = new_element_id,
            .old_element_id = old_element_id,
            .data = data,
        });
    }
}

auto swap_and_delete_single_element(Circuit& circuit, MessageSender sender,
                                    element_id_t& element_id) -> void {
    auto& schematic = circuit.schematic();
    auto& layout = circuit.layout();

    if (layout.display_state(element_id) != display_state_t::new_temporary) [[unlikely]] {
        throw_exception("can only delete temporary objects");
    }

    sender.submit(info_message::ElementDeleted {element_id});

    // delete in underlying
    auto last_id = schematic.swap_and_delete_element(element_id);
    {
        auto last_id_2 = layout.swap_and_delete_element(element_id);
        if (last_id_2 != last_id) {
            throw_exception("Returned id's during deletion are not the same.");
        }
    }

    if (element_id != last_id) {
        notify_element_id_change(circuit, sender, element_id, last_id);
    }
    element_id = null_element;
}

//
// Connection Handler
//

//
// Logic Item Handlers
//

auto add_standard_logic_item(State state, Selection& selection,
                             StandardLogicAttributes attributes,
                             InsertionMode insertion_mode) -> void {
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
    auto element_id = state.layout.add_logic_element(
        point_t {0, 0}, attributes.orientation, display_state_t::new_temporary);
    {
        const auto element = state.schematic.add_element({
            .element_type = attributes.type,
            .input_count = attributes.input_count,
            .output_count = 1,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }
    state.sender.submit(info_message::ElementCreated {element_id});
    selection.add_element(element_id);

    // validates our position
    move_or_delete_logic_item(state, element_id,            //
                              attributes.position.x.value,  //
                              attributes.position.y.value);
    if (element_id) {
        change_logic_item_insertion_mode(state, element_id, insertion_mode);
    }
}

auto add_placeholder_element(Circuit& circuit) -> element_id_t {
    constexpr static auto connector_delay
        = delay_t {Schematic::defaults::wire_delay_per_distance.value / 2};

    const auto element_id = circuit.layout().add_placeholder(display_state_t::normal);
    {
        const auto element = circuit.schematic().add_element(Schematic::NewElementData {
            .element_type = ElementType::placeholder,
            .input_count = 1,
            .output_count = 0,
            .history_length = connector_delay,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }

    return element_id;
}

auto is_logic_item_position_representable(const Circuit& circuit, element_id_t element_id,
                                          int x, int y) -> bool {
    if (!is_representable(x, y)) {
        return false;
    }
    const auto position = point_t {grid_t {x}, grid_t {y}};

    auto data = to_layout_calculation_data(circuit, element_id);
    data.position = position;

    return is_representable(data);
}

auto move_or_delete_logic_item(State state, element_id_t& element_id, int x, int y)
    -> void {
    // only temporary items can be freely moved
    if (state.layout.display_state(element_id) != display_state_t::new_temporary)
        [[unlikely]] {
        throw_exception("Only temporary items can be freely moded.");
    }

    if (!is_logic_item_position_representable(state.circuit, element_id, x, y)) {
        change_logic_item_insertion_mode(state, element_id, InsertionMode::temporary);
        swap_and_delete_single_element(state.circuit, state.sender, element_id);
        return;
    }

    const auto position = point_t {grid_t {x}, grid_t {y}};
    state.layout.set_position(element_id, position);
}

// mode change helpers

auto add_and_connect_placeholder(Circuit& circuit, Schematic::Output output)
    -> element_id_t {
    const auto placeholder_id = add_placeholder_element(circuit);
    const auto input
        = circuit.schematic().element(placeholder_id).input(connection_id_t {0});
    output.connect(input);

    return placeholder_id;
}

auto disconnect_inputs_and_add_placeholders(Circuit& circuit, element_id_t element_id)
    -> void {
    auto& schematic = circuit.schematic();

    if (schematic.element(element_id).is_placeholder()) {
        return;
    }

    for (const auto input : schematic.element(element_id).inputs()) {
        if (input.has_connected_element()) {
            add_and_connect_placeholder(circuit, input.connected_output());
        }
    }
}

auto disconnect_outputs_and_remove_placeholders(Circuit& circuit, MessageSender sender,
                                                element_id_t& element_id) -> void {
    auto& schematic = circuit.schematic();

    auto delete_queue = delete_queue_t {};

    for (auto output : schematic.element(element_id).outputs()) {
        if (output.has_connected_element()) {
            const auto connected_element = output.connected_element();

            if (connected_element.is_placeholder()) {
                delete_queue.push_back(connected_element.element_id());
            }
            output.clear_connection();
        }
    }

    // const auto handle = _hack_element_handle(element_id);
    throw_exception("implement");
    swap_and_delete_multiple_elements(circuit, sender, delete_queue);
    // element_id = handle.element();
}

auto add_missing_placeholders_for_outputs(Circuit& circuit, element_id_t element_id)
    -> void {
    for (const auto output : circuit.schematic().element(element_id).outputs()) {
        if (!output.has_connected_element()) {
            add_and_connect_placeholder(circuit, output);
        }
    }
}

namespace {
struct connector_data_t {
    // element_id and connection_id
    connection_t connection_data;
    // position of the connector
    point_t position;
    // orientation of the connector
    orientation_t orientation;
};
}  // namespace

template <bool IsInput>
auto connect_connector(connector_data_t connector,
                       const ConnectionCache<IsInput>& connection_cache,
                       Schematic& schematic) -> std::optional<element_id_t> {
    auto unused_placeholder_id = std::optional<element_id_t> {};
    auto connection = to_connection<!IsInput>(schematic, connector.connection_data);

    // pre-conditions
    if (connection.has_connected_element()) [[unlikely]] {
        throw_exception("Connections needs to be unconnected.");
    }

    // find connection at position
    if (const auto entry = connection_cache.find(connector.position, schematic)) {
        const auto found_connection = entry->first;
        const auto found_orientation = entry->second;

        if (found_connection.has_connected_element()) {
            if (!found_connection.connected_element().is_placeholder()) [[unlikely]] {
                throw_exception(
                    "Connection is already connected at "
                    "this location.");
            }
            // mark placeholder for deletion
            unused_placeholder_id = found_connection.connected_element_id();
        }
        if (!orientations_compatible(connector.orientation, found_orientation)) {
            throw_exception(
                "Connection have incompatible "
                "orientations.");
        }

        // make connection in schematic
        connection.connect(found_connection);
    }

    return unused_placeholder_id;
}

[[nodiscard]] auto connect_element(State state, element_id_t element_id)
    -> delete_queue_t {
    auto disconnected_placeholders = delete_queue_t {};
    auto add_if_valid = [&](std::optional<element_id_t> placeholder_id) {
        if (placeholder_id) {
            disconnected_placeholders.push_back(*placeholder_id);
        }
    };

    const auto data = to_layout_calculation_data(state.circuit, element_id);

    // inputs
    iter_input_location_and_id(
        data, [&, element_id](connection_id_t input_id, point_t position,
                              orientation_t orientation) {
            auto input = connector_data_t {{element_id, input_id}, position, orientation};
            // connect the input using the
            // output_connections cache
            const auto placeholder_id
                = connect_connector(input, state.cache.output_cache(), state.schematic);
            add_if_valid(placeholder_id);
            return true;
        });

    // outputs
    iter_output_location_and_id(data, [&, element_id](connection_id_t output_id,
                                                      point_t position,
                                                      orientation_t orientation) mutable {
        auto output = connector_data_t {{element_id, output_id}, position, orientation};
        // connect the output using the
        // input_connections cache
        const auto placeholder_id
            = connect_connector(output, state.cache.input_cache(), state.schematic);
        add_if_valid(placeholder_id);
        return true;
    });

    return disconnected_placeholders;
}

auto insert_element(State state, element_id_t& element_id) {
    const auto disconnected_placeholders = connect_element(state, element_id);
    add_missing_placeholders_for_outputs(state.circuit, element_id);

    // this may change our element_id
    // auto handle = _hack_element_handle(element_id);
    if (disconnected_placeholders.empty()) {
        return;
    }
    throw_exception("implement");
    swap_and_delete_multiple_elements(state.circuit, state.sender,
                                      disconnected_placeholders);
    // element_id = handle.element();
}

// mode change

auto element_change_temporary_to_colliding(State state, element_id_t& element_id)
    -> void {
    if (state.layout.display_state(element_id) != display_state_t::new_temporary)
        [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    const auto data = to_layout_calculation_data(state.circuit, element_id);

    if (state.cache.is_element_colliding(data)) {
        state.layout.set_display_state(element_id, display_state_t::new_colliding);
    } else {
        insert_element(state, element_id);
        state.layout.set_display_state(element_id, display_state_t::new_valid);
        state.sender.submit(info_message::LogicItemInserted {element_id, data});
    }
};

auto element_change_colliding_to_insert(Circuit& circuit, MessageSender sender,
                                        element_id_t& element_id) -> void {
    auto& layout = circuit.layout();
    const auto display_state = layout.display_state(element_id);

    if (display_state == display_state_t::new_valid) {
        layout.set_display_state(element_id, display_state_t::normal);
        return;
    }

    if (display_state == display_state_t::new_colliding) [[likely]] {
        // we can only delete temporary elements
        layout.set_display_state(element_id, display_state_t::new_temporary);
        swap_and_delete_single_element(circuit, sender, element_id);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto element_change_insert_to_colliding(Layout& layout, element_id_t& element_id)
    -> void {
    if (layout.display_state(element_id) != display_state_t::normal) [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    layout.set_display_state(element_id, display_state_t::new_valid);
};

auto element_change_colliding_to_temporary(Circuit& circuit, MessageSender sender,
                                           element_id_t& element_id) -> void {
    auto& layout = circuit.layout();
    const auto display_state = layout.display_state(element_id);

    if (display_state == display_state_t::new_valid) {
        const auto data = to_layout_calculation_data(circuit, element_id);
        sender.submit(info_message::LogicItemUninserted {element_id, data});
        layout.set_display_state(element_id, display_state_t::new_temporary);

        disconnect_inputs_and_add_placeholders(circuit, element_id);
        disconnect_outputs_and_remove_placeholders(circuit, sender, element_id);
        return;
    }

    if (display_state == display_state_t::new_colliding) {
        layout.set_display_state(element_id, display_state_t::new_temporary);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto change_logic_item_insertion_mode(State state, element_id_t& element_id,
                                      InsertionMode new_insertion_mode) -> void {
    if (!state.schematic.element(element_id).is_logic_item()) [[unlikely]] {
        throw_exception("only works on logic elements");
    }
    const auto old_insertion_mode
        = to_insertion_mode(state.layout.display_state(element_id));

    if (old_insertion_mode == new_insertion_mode) {
        return;
    }

    if (old_insertion_mode == InsertionMode::temporary) {
        element_change_temporary_to_colliding(state, element_id);
    }
    if (new_insertion_mode == InsertionMode::insert_or_discard) {
        element_change_colliding_to_insert(state.circuit, state.sender, element_id);
    }

    if (old_insertion_mode == InsertionMode::insert_or_discard) {
        element_change_insert_to_colliding(state.layout, element_id);
    }
    if (new_insertion_mode == InsertionMode::temporary) {
        element_change_colliding_to_temporary(state.circuit, state.sender, element_id);
    }
}

//
// Wire Editing
//

/*


auto line_uninserted_TODO() {
    for (auto&& index : layout.segment_tree(element_id).indices(element_id)) {
        sender.submit(info_message::SegmentUninserted {index});
    }
}

auto WireEditor::add_connected_line(point_t p0, point_t p1, LineSegmentType
segment_type, InsertionMode insertion_mode) -> selection_handle_t { auto
selection_handle = editable_circuit_.create_selection();

    // TODO what with p0 == p1

    switch (segment_type) {
        using enum LineSegmentType;

        case horizontal_first: {
            const auto pm = point_t {p1.x, p0.y};
            if (p0.x != pm.x) {
                add_line_segment(line_t {p0, pm}, insertion_mode,
selection_handle.get());
            }
            if (pm.y != p1.y) {
                add_line_segment(line_t {pm, p1}, insertion_mode,
selection_handle.get());
            }
            break;
        }
        case vertical_first: {
            const auto pm = point_t {p0.x, p1.y};
            if (p0.y != pm.y) {
                add_line_segment(line_t {p0, pm}, insertion_mode,
selection_handle.get());
            }
            if (pm.x != p1.x) {
                add_line_segment(line_t {pm, p1}, insertion_mode,
selection_handle.get());
            }
            break;
        }
    }

    return selection_handle;
}

auto get_segment(const Layout& layout, segment_t segment) -> segment_info_t {
    return layout.segment_tree(segment.element_id).segment(segment.segment_index);
}

auto get_segment_line(const Layout& layout, segment_t segment) -> line_t {
    return get_segment(layout, segment).line;
}

auto WireEditor::set_segment_point_types(
    std::initializer_list<const std::pair<segment_t, SegmentPointType>> data,
    point_t position) -> void {
    // remove cache
    for (auto&& [segment, point_type] : data) {
        // TODO only do this for some insertion modes
        cache_remove(segment.element_id, segment.segment_index);
    }

    // update segments
    for (auto&& [segment, point_type] : data) {
        auto&& m_tree = layout_.modifyable_segment_tree(segment.element_id);
        auto new_segment = m_tree.segment(segment.segment_index);

        if (new_segment.line.p0 == position) {
            new_segment.p0_type = point_type;
        } else if (new_segment.line.p1 == position) {
            new_segment.p1_type = point_type;
        } else {
            throw_exception("Position needs to be an endpoint of the given segment.");
        }

        m_tree.update_segment(segment.segment_index, new_segment,
                              display_state_t::normal);
    }

    // add to cache
    for (auto&& [segment, point_type] : data) {
        // TODO only do this for some insertion modes
        cache_insert(segment.element_id, segment.segment_index);
    }
}

auto sort_lines_with_endpoints_last(std::span<std::pair<line_t, segment_t>> lines,
                                    point_t point) -> void {
    std::ranges::sort(lines, {}, [point](std::pair<line_t, segment_t> item) {
        return is_endpoint(point, item.first);
    });
}

auto merge_parallel_segments(segment_info_t segment_info_0, segment_info_t
segment_info_1)
    -> segment_info_t {
    const auto [a, b] = order_points(segment_info_0, segment_info_1);

    if (a.line.p1 != b.line.p0) [[unlikely]] {
        throw_exception("segments need to have common shared point");
    }

    return segment_info_t {
        .line = line_t {a.line.p0, b.line.p1},
        .p0_type = a.p0_type,
        .p1_type = b.p1_type,
    };
}

auto WireEditor::merge_line_segments(element_id_t element_id, segment_index_t index0,
                                     segment_index_t index1) -> void {
    if (index0 == index1) [[unlikely]] {
        throw_exception("Cannot merge the same segments.");
    }
    if (index0 > index1) {
        std::swap(index0, index1);
    }

    auto& m_tree = layout_.modifyable_segment_tree(element_id);
    const auto last_index = m_tree.last_index();

    if (!is_inserted(m_tree.display_state(index0))
        || !is_inserted(m_tree.display_state(index1))) [[unlikely]] {
        throw_exception("Can only merge inserted segments.");
    }

    // merged segment
    const auto merged_segment
        = merge_parallel_segments(m_tree.segment(index0), m_tree.segment(index1));

    // remove from cache
    cache_remove(element_id, index0);
    cache_remove(element_id, index1);
    if (index1 != last_index) {
        cache_remove(element_id, last_index);
    }

    // merge
    m_tree.update_segment(index0, merged_segment, display_state_t::normal);
    m_tree.swap_and_delete_segment(index1);

    // add back to cache
    cache_insert(element_id, index0);
    if (index1 != last_index) {
        cache_insert(element_id, index1);
    }
}

auto all_collision_condered(const SegmentTree& tree,
                            SpatialTree::queried_segments_t result) -> bool {
    return std::ranges::all_of(result, [&](segment_t value) {
        return value.segment_index == null_segment_index
               || is_inserted(tree.display_state(value.segment_index));
    });
}

auto WireEditor::fix_line_segments(point_t position) -> void {
    // TODO rename to segments
    const auto segment = spatial_cache_.query_line_segments(position);
    const auto segment_count = get_segment_count(segment);

    if (segment_count == 0) [[unlikely]] {
        throw_exception("Could not find any segments at position.");
    }
    if (!all_same_element_id(segment)) [[unlikely]] {
        throw_exception("All segments need to belong to the same segment tree.");
    }
    if (const auto tree = layout_.segment_tree(segment.at(0).element_id);
        !all_collision_condered(tree, segment)) {
        throw_exception("Can only fix collision considered segments.");
    }

    if (segment_count == 1) {
        set_segment_point_types(
            {
                std::pair {segment.at(0), SegmentPointType::output},
            },
            position);
        return;
    }

    if (segment_count == 2) {
        auto lines = std::array {
            std::pair {get_segment_line(layout_, segment.at(0)), segment.at(0)},
            std::pair {get_segment_line(layout_, segment.at(1)), segment.at(1)},
        };
        sort_lines_with_endpoints_last(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            const auto cross_point_type = is_horizontal(lines.at(1).first)
                                              ?
SegmentPointType::cross_point_horizontal : SegmentPointType::cross_point_vertical;
            set_segment_point_types(
                {
                    std::pair {lines.at(1).second, cross_point_type},
                },
                position);
            return;
        }

        const auto horizontal_0 = is_horizontal(lines.at(0).first);
        const auto horizontal_1 = is_horizontal(lines.at(1).first);
        const auto parallel = horizontal_0 == horizontal_1;

        if (parallel) {
            merge_line_segments(segment.at(0).element_id, segment.at(0).segment_index,
                                segment.at(1).segment_index);
            return;
        }

        // handle corner
        set_segment_point_types(
            {
                std::pair {segment.at(0), SegmentPointType::colliding_point},
                std::pair {segment.at(1), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    if (segment_count == 3) {
        auto lines = std::array {
            std::pair {get_segment_line(layout_, segment.at(0)), segment.at(0)},
            std::pair {get_segment_line(layout_, segment.at(1)), segment.at(1)},
            std::pair {get_segment_line(layout_, segment.at(2)), segment.at(2)},
        };
        sort_lines_with_endpoints_last(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            const auto cross_point_type = is_horizontal(lines.at(2).first)
                                              ?
SegmentPointType::cross_point_horizontal : SegmentPointType::cross_point_vertical;
            set_segment_point_types(
                {
                    std::pair {lines.at(1).second, SegmentPointType::shadow_point},
                    std::pair {lines.at(2).second, cross_point_type},
                },
                position);
        } else {
            set_segment_point_types(
                {
                    std::pair {segment.at(0), SegmentPointType::colliding_point},
                    std::pair {segment.at(1), SegmentPointType::shadow_point},
                    std::pair {segment.at(2), SegmentPointType::visual_cross_point},
                },
                position);
        }
        return;
    }

    if (segment_count == 4) {
        set_segment_point_types(
            {
                std::pair {segment.at(0), SegmentPointType::colliding_point},
                std::pair {segment.at(1), SegmentPointType::shadow_point},
                std::pair {segment.at(2), SegmentPointType::shadow_point},
                std::pair {segment.at(3), SegmentPointType::visual_cross_point},
            },
            position);
        return;
    }

    throw_exception("unexpected unhandeled case");
}

auto WireEditor::add_line_segment(line_t line, InsertionMode insertion_mode,
                                  Selection* selection) -> void {
    if (insertion_mode != InsertionMode::insert_or_discard) {
        throw_exception("Not implemented.");
    }

    if (is_colliding(line)) {
        fmt::print("Collision failed\n");
        return;
    }

    const auto colliding_id_0 = collision_cache_.get_first_wire(line.p0);
    auto colliding_id_1 = collision_cache_.get_first_wire(line.p1);

    auto tree_handle = editable_circuit_.element_handle();

    if (colliding_id_0) {
        tree_handle.set_element(colliding_id_0);
    }

    if (colliding_id_1) {
        if (!tree_handle) {
            tree_handle.set_element(colliding_id_1);
        } else {
            // merge two trees
            const auto tree_copy = SegmentTree {layout_.segment_tree(colliding_id_1)};
            editable_circuit_.swap_and_delete_single_element(colliding_id_1);

            const auto element_id = tree_handle.element();
            auto&& m_tree = layout_.modifyable_segment_tree(element_id);

            auto first_index = m_tree.add_tree(tree_copy);
            for (auto segment_index : range(first_index, ++m_tree.last_index())) {
                cache_insert(element_id, segment_index);
            }
        }
    }

    if (!tree_handle) {
        // create new empty tree
        const auto element_id = layout_.add_line_tree(SegmentTree {});
        {
            const auto element = schematic_.add_element({
                .element_type = ElementType::wire,
                .input_count = 0,
                .output_count = 0,
            });
            if (element.element_id() != element_id) [[unlikely]] {
                throw_exception("Added element ids don't match.");
            }
        }
        editable_circuit_.key_insert(element_id);
        tree_handle.set_element(element_id);
    }

    {
        // insert new segment with dummy endpoints
        const auto element_id = tree_handle.element();
        auto&& m_tree = layout_.modifyable_segment_tree(element_id);

        const auto segment = segment_info_t {
            .line = line,
            .p0_type = SegmentPointType::shadow_point,
            .p1_type = SegmentPointType::shadow_point,
        };

        const auto segment_index = m_tree.add_segment(segment,
display_state_t::normal); cache_insert(element_id, segment_index);

        if (selection != nullptr) {
            const auto segment_part = get_segment_part(segment.line);
            selection->add_segment(segment_t {element_id, segment_index},
segment_part);
        }
    }

    // now fix all endpoints at given positions
    fix_line_segments(line.p0);
    fix_line_segments(line.p1);

    // TODO insertion mode change

#ifndef NDEBUG
    layout_.segment_tree(tree_handle.element()).validate();
#endif
}

auto WireEditor::is_colliding(line_t line) const -> bool {
    // TODO connections colliding

    return collision_cache_.is_colliding(line);
}

auto WireEditor::cache_insert(element_id_t element_id, segment_index_t segment_index)
    -> void {
    const auto segment = layout_.segment_tree(element_id).segment(segment_index);

    input_connections_.insert(element_id, segment);
    output_connections_.insert(element_id, segment);
    collision_cache_.insert(element_id, segment);
    spatial_cache_.insert(element_id, segment.line, segment_index);
}

auto WireEditor::cache_remove(element_id_t element_id, segment_index_t segment_index)
    -> void {
    const auto segment = layout_.segment_tree(element_id).segment(segment_index);

    input_connections_.remove(element_id, segment);
    output_connections_.remove(element_id, segment);
    collision_cache_.remove(element_id, segment);
    spatial_cache_.remove(element_id, segment.line, segment_index);
}

*/

}  // namespace editable_circuit
}  // namespace logicsim
