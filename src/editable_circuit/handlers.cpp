#include "editable_circuit/handlers.h"

#include "editable_circuit/caches.h"
#include "editable_circuit/selection.h"
#include "geometry.h"
#include "layout_calculations.h"
#include "scene.h"

namespace logicsim {

namespace editable_circuit {

//
// Deletion Handling
//

auto is_wire_with_segments(const Circuit& circuit, element_id_t element_id) -> bool {
    const auto element = circuit.schematic().element(element_id);
    return element.is_wire() && !circuit.layout().segment_tree(element_id).empty();
}

auto notify_element_deleted(const Schematic& schematic, MessageSender sender,
                            element_id_t element_id) {
    const auto element = schematic.element(element_id);

    if (element.is_logic_item()) {
        sender.submit(info_message::LogicItemDeleted {element_id});
    }
}

auto notify_element_id_change(const Circuit& circuit, MessageSender sender,
                              element_id_t new_element_id, element_id_t old_element_id) {
    const auto& layout = circuit.layout();
    const auto& schematic = circuit.schematic();
    const auto element = schematic.element(new_element_id);

    if (element.is_placeholder()) {
        return;
    }

    const bool inserted = is_inserted(circuit, new_element_id);

    if (element.is_logic_item()) {
        sender.submit(info_message::LogicItemIdUpdated {
            .new_element_id = new_element_id,
            .old_element_id = old_element_id,
        });
    }

    if (element.is_logic_item() && inserted) {
        const auto data = to_layout_calculation_data(circuit, new_element_id);

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

auto swap_elements(Circuit& circuit, MessageSender sender, element_id_t element_id_0,
                   element_id_t element_id_1) -> void {
    if (element_id_0 == element_id_1) {
        return;
    }

    if (is_inserted(circuit, element_id_0) && is_inserted(circuit, element_id_1))
        [[unlikely]] {
        // we might need element delete and uninsert to prevent conflicts
        // or we need to introduce ElementSwapped messages
        throw_exception("not implemented");
    }

    circuit.swap_elements(element_id_0, element_id_1);
    notify_element_id_change(circuit, sender, element_id_0, element_id_1);
    notify_element_id_change(circuit, sender, element_id_1, element_id_0);
}

auto swap_and_delete_single_element_private(Circuit& circuit, MessageSender sender,
                                            element_id_t& element_id,
                                            element_id_t* preserve_element = nullptr)
    -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }

    auto& schematic = circuit.schematic();
    auto& layout = circuit.layout();

    if (layout.display_state(element_id) != display_state_t::new_temporary) [[unlikely]] {
        throw_exception("can only delete temporary objects");
    }
    if (is_wire_with_segments(circuit, element_id)) [[unlikely]] {
        throw_exception("can't delete wires with segments");
    }

    notify_element_deleted(schematic, sender, element_id);

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

    if (preserve_element != nullptr) {
        if (*preserve_element == element_id) {
            *preserve_element = null_element;
        } else if (*preserve_element == last_id) {
            *preserve_element = element_id;
        }
    }

    element_id = null_element;
}

auto swap_and_delete_single_element(Circuit& circuit, MessageSender sender,
                                    element_id_t& element_id,
                                    element_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "swap_and_delete_single_element(element_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            circuit, element_id, fmt_ptr(preserve_element));
    }
    swap_and_delete_single_element_private(circuit, sender, element_id, preserve_element);
}

auto swap_and_delete_multiple_elements_private(Circuit& circuit, MessageSender sender,
                                               std::span<const element_id_t> element_ids,
                                               element_id_t* preserve_element) -> void {
    // sort descending, so we don't invalidate our ids
    auto sorted_ids = delete_queue_t {element_ids.begin(), element_ids.end()};
    std::ranges::sort(sorted_ids, std::greater<> {});

    for (auto element_id : sorted_ids) {
        swap_and_delete_single_element_private(circuit, sender, element_id,
                                               preserve_element);
    }
}

auto swap_and_delete_multiple_elements(Circuit& circuit, MessageSender sender,
                                       std::span<const element_id_t> element_ids,
                                       element_id_t* preserve_element) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "swap_and_delete_multiple_elements(element_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            circuit, element_ids, fmt_ptr(preserve_element));
    }
    swap_and_delete_multiple_elements_private(circuit, sender, element_ids,
                                              preserve_element);
}

auto delete_disconnected_placeholders(Circuit& circuit, MessageSender sender,
                                      std::span<const element_id_t> placeholder_ids,
                                      element_id_t* preserve_element = nullptr) {
    for (auto placeholder_id : placeholder_ids) {
        circuit.layout().set_display_state(placeholder_id,
                                           display_state_t::new_temporary);
    }
    swap_and_delete_multiple_elements_private(circuit, sender, placeholder_ids,
                                              preserve_element);
}

//
// Logic Item Handling
//

auto StandardLogicAttributes::format() const -> std::string {
    return fmt::format("{{{}, input_count = {}, {}, {}}}", type, input_count, position,
                       orientation);
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

auto is_logic_item_position_representable_private(const Circuit& circuit,
                                                  element_id_t element_id, int x, int y)
    -> bool {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }
    if (!is_representable(x, y)) {
        return false;
    }
    const auto position = point_t {grid_t {x}, grid_t {y}};

    auto data = to_layout_calculation_data(circuit, element_id);
    data.position = position;

    return is_representable(data);
}

auto is_logic_item_position_representable(const Circuit& circuit, element_id_t element_id,
                                          int x, int y) -> bool {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "is_logic_item_position_representable(element_id = {}, x = {}, y = {});\n"
            "==========================================================\n\n",
            circuit, element_id, x, y);
    }
    return is_logic_item_position_representable_private(circuit, element_id, x, y);
}

auto move_or_delete_logic_item_private(State state, element_id_t& element_id, int x,
                                       int y) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }
    if (state.layout.display_state(element_id) != display_state_t::new_temporary)
        [[unlikely]] {
        throw_exception("Only temporary items can be freely moded.");
    }

    if (!is_logic_item_position_representable_private(state.circuit, element_id, x, y)) {
        swap_and_delete_single_element_private(state.circuit, state.sender, element_id);
        return;
    }

    const auto position = point_t {grid_t {x}, grid_t {y}};
    state.layout.set_position(element_id, position);
}

auto move_or_delete_logic_item(State state, element_id_t& element_id, int x, int y)
    -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "move_or_delete_logic_item(element_id = {}, x = {}, y = {});\n"
            "==========================================================\n\n",
            state.circuit, element_id, x, y);
    }
    move_or_delete_logic_item_private(state, element_id, x, y);
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

    for (const auto input : schematic.element(element_id).inputs()) {
        if (input.has_connected_element()) {
            add_and_connect_placeholder(circuit, input.connected_output());
        }
    }
}

auto disconnect_outputs_and_remove_placeholders(Circuit& circuit, MessageSender sender,
                                                element_id_t& element_id) -> void {
    auto& schematic = circuit.schematic();

    auto disconnected_placeholders = delete_queue_t {};

    for (auto output : schematic.element(element_id).outputs()) {
        if (output.has_connected_element()) {
            const auto connected_element = output.connected_element();

            if (connected_element.is_placeholder()) {
                disconnected_placeholders.push_back(connected_element.element_id());
            }
            output.clear_connection();
        }
    }

    delete_disconnected_placeholders(circuit, sender, disconnected_placeholders,
                                     &element_id);
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

auto connect_element(State state, element_id_t& element_id) -> void {
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
            // connect the input using the output_connections cache
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
        // connect the output using the  input_connections cache
        const auto placeholder_id
            = connect_connector(output, state.cache.input_cache(), state.schematic);
        add_if_valid(placeholder_id);
        return true;
    });

    delete_disconnected_placeholders(state.circuit, state.sender,
                                     disconnected_placeholders, &element_id);
}

auto insert_logic_item(State state, element_id_t& element_id) {
    // we assume there will be no collision
    connect_element(state, element_id);
    add_missing_placeholders_for_outputs(state.circuit, element_id);
}

// mode change

auto is_circuit_item_colliding(const Circuit& circuit, const CacheProvider& cache,
                               element_id_t element_id) {
    const auto data = to_layout_calculation_data(circuit, element_id);
    return cache.is_element_colliding(data);
}

auto notify_circuit_item_inserted(const Circuit& circuit, MessageSender sender,
                                  element_id_t element_id) {
    const auto data = to_layout_calculation_data(circuit, element_id);
    sender.submit(info_message::LogicItemInserted {element_id, data});
}

auto element_change_temporary_to_colliding(State state, element_id_t& element_id)
    -> void {
    if (state.layout.display_state(element_id) != display_state_t::new_temporary)
        [[unlikely]] {
        throw_exception("element is not in the right state.");
    }

    if (is_circuit_item_colliding(state.circuit, state.cache, element_id)) {
        state.layout.set_display_state(element_id, display_state_t::new_colliding);
    } else {
        insert_logic_item(state, element_id);
        state.layout.set_display_state(element_id, display_state_t::new_valid);
        notify_circuit_item_inserted(state.circuit, state.sender, element_id);
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
        swap_and_delete_single_element_private(circuit, sender, element_id);
        return;
    }

    throw_exception("element is not in the right state.");
};

auto element_change_insert_to_colliding(Layout& layout, element_id_t element_id) -> void {
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

auto change_logic_item_insertion_mode_private(State state, element_id_t& element_id,
                                              InsertionMode new_mode) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("element id is invalid");
    }
    if (!state.schematic.element(element_id).is_logic_item()) [[unlikely]] {
        throw_exception("only works on logic elements");
    }

    const auto old_mode = to_insertion_mode(state.layout.display_state(element_id));
    if (old_mode == new_mode) {
        return;
    }

    if (old_mode == InsertionMode::temporary) {
        element_change_temporary_to_colliding(state, element_id);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        element_change_colliding_to_insert(state.circuit, state.sender, element_id);
    }
    if (old_mode == InsertionMode::insert_or_discard) {
        element_change_insert_to_colliding(state.layout, element_id);
    }
    if (new_mode == InsertionMode::temporary) {
        element_change_colliding_to_temporary(state.circuit, state.sender, element_id);
    }
}

auto change_logic_item_insertion_mode(State state, element_id_t& element_id,
                                      InsertionMode new_mode) -> void {
    if constexpr (DEBUG_PRINT_HANDLER_INPUTS) {
        fmt::print(
            "\n==========================================================\n{}\n"
            "change_logic_item_insertion_mode(element_id = {}, new_mode = {});\n"
            "==========================================================\n\n",
            state.circuit, element_id, new_mode);
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
    state.sender.submit(info_message::LogicItemCreated {element_id});

    // validates our position
    move_or_delete_logic_item_private(state, element_id,            //
                                      attributes.position.x.value,  //
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
            state.circuit, attributes, insertion_mode);
    }
    return add_standard_logic_item_private(state, attributes, insertion_mode);
}

//
// Wire Handling
//

// aggregates

auto is_wire_aggregate(const Schematic& schematic, const Layout& layout,
                       element_id_t element_id, display_state_t display_state) -> bool {
    return schematic.element(element_id).is_wire()
           && layout.display_state(element_id) == display_state;
}

auto add_new_temporary_wire_element(Circuit& circuit, MessageSender sender)
    -> element_id_t {
    const auto element_id
        = circuit.layout().add_line_tree(display_state_t::new_temporary);
    {
        const auto element = circuit.schematic().add_element({
            .element_type = ElementType::wire,
            .input_count = 0,
            .output_count = 0,
        });
        if (element.element_id() != element_id) [[unlikely]] {
            throw_exception("Added element ids don't match.");
        }
    }
    return element_id;
}

auto find_wire(const Circuit& circuit, display_state_t display_state) -> element_id_t {
    const auto& layout = circuit.layout();
    const auto& schematic = circuit.schematic();

    // test begin
    // if (display_state == display_state_t::new_temporary) {
    //    return null_element;
    //}
    // test end

    const auto element_ids = layout.element_ids();
    const auto it
        = std::ranges::find_if(element_ids, [&](element_id_t element_id) -> bool {
              return is_wire_aggregate(schematic, layout, element_id, display_state);
          });
    return it == element_ids.end() ? null_element : *it;
}

auto create_aggregate_tree_at(Circuit& circuit, MessageSender sender,
                              display_state_t display_state, element_id_t target_id)
    -> void {
    auto element_id = find_wire(circuit, display_state);

    if (!element_id) {
        element_id = add_new_temporary_wire_element(circuit, sender);
        circuit.layout().set_display_state(element_id, display_state);
    }

    if (element_id != target_id) {
        swap_elements(circuit, sender, element_id, target_id);
    }
}

constexpr inline static auto TEMPORARY_AGGREGATE_ID = element_id_t {0};
constexpr inline static auto COLLIDING_AGGREGATE_ID = element_id_t {1};

auto create_aggregate_wires(Circuit& circuit, MessageSender sender) -> void {
    using enum display_state_t;
    create_aggregate_tree_at(circuit, sender, new_temporary, TEMPORARY_AGGREGATE_ID);
    create_aggregate_tree_at(circuit, sender, new_colliding, COLLIDING_AGGREGATE_ID);
}

auto get_or_create_aggregate(Circuit& circuit, MessageSender sender,
                             display_state_t display_state) -> element_id_t {
    using enum display_state_t;
    const auto& layout = circuit.layout();
    const auto& schematic = circuit.schematic();

    // temporary
    if (display_state == new_temporary) {
        if (layout.element_count() <= TEMPORARY_AGGREGATE_ID.value
            || !is_wire_aggregate(schematic, layout, TEMPORARY_AGGREGATE_ID,
                                  new_temporary)) {
            create_aggregate_wires(circuit, sender);
        }
        return TEMPORARY_AGGREGATE_ID;
    }

    // colliding
    else if (display_state == new_colliding) {
        if (layout.element_count() <= COLLIDING_AGGREGATE_ID.value
            || !is_wire_aggregate(schematic, layout, COLLIDING_AGGREGATE_ID,
                                  new_temporary)) {
            create_aggregate_wires(circuit, sender);
        }
        return COLLIDING_AGGREGATE_ID;
    }

    throw_exception("display state has no aggregate");
}

auto add_segment_to_tree(Circuit& circuit, MessageSender sender, element_id_t element_id,
                         ordered_line_t line) -> segment_part_t {
    // insert new segment
    auto& m_tree = circuit.layout().modifyable_segment_tree(element_id);

    const auto segment_info = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };
    const auto segment_index = m_tree.add_segment(segment_info);
    const auto segment = segment_t {element_id, segment_index};

    // messages
    sender.submit(info_message::SegmentCreated {segment});
    if (is_inserted(circuit, element_id)) {
        sender.submit(info_message::SegmentInserted {segment, segment_info});
    }

    return segment_part_t {segment, to_part(line)};
}

auto add_segment_to_aggregate(Circuit& circuit, MessageSender sender, ordered_line_t line,
                              display_state_t aggregate_type) -> segment_part_t {
    const auto element_id = get_or_create_aggregate(circuit, sender, aggregate_type);
    return add_segment_to_tree(circuit, sender, element_id, line);
}

// insertion mode changing

auto is_wire_colliding(const CacheProvider& cache, ordered_line_t line) -> bool {
    // TODO connections colliding
    return cache.collision_cache().is_colliding(line);
}

auto get_display_states(const Layout& layout, segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t> {
    using enum display_state_t;

    const auto& tree = layout.segment_tree(segment_part.segment.element_id);
    const auto tree_state = layout.display_state(segment_part.segment.element_id);

    // our aggregates
    if (tree_state == new_temporary || tree_state == new_colliding) {
        return std::make_pair(tree_state, tree_state);
    }

    const auto&& valid_parts = tree.valid_parts(segment_part.segment.segment_index);
    const auto part_valid = is_part_included(valid_parts, segment_part.part);

    switch (part_valid) {
        using enum InclusionResult;

        case fully_included:
            return std::make_pair(new_valid, new_valid);

        case not_included:
            return std::make_pair(normal, normal);

        case partially_overlapping:
            return std::make_pair(new_valid, normal);
    }

    throw_exception("unknown InclusionResult state");
}

auto get_insertion_modes(const Layout& layout, segment_part_t segment_part)
    -> std::pair<InsertionMode, InsertionMode> {
    const auto display_states = get_display_states(layout, segment_part);

    return std::make_pair(to_insertion_mode(display_states.first),
                          to_insertion_mode(display_states.second));
}

auto delete_inserted_tree_segments(Circuit& circuit, MessageSender sender,
                                   element_id_t element_id) -> void {
    if (!is_inserted(circuit, element_id) || !is_wire(circuit, element_id)) [[unlikely]] {
        throw_exception("can only delete from inserted wires");
    }

    auto& m_tree = circuit.layout().modifyable_segment_tree(element_id);

    for (auto segment_index = m_tree.last_index(); segment_index >= segment_index_t {0};
         --segment_index) {
        const auto segment = segment_t {element_id, segment_index};
        const auto segment_info = m_tree.segment_info(segment_index);
        const auto part = to_part(segment_info.line);

        m_tree.swap_and_delete_segment(segment_index);
        sender.submit(info_message::SegmentUninserted {segment, segment_info});
        sender.submit(info_message::SegmentPartDeleted {segment_part_t {segment, part}});
    }
}

auto merge_trees(Circuit& circuit, MessageSender sender, element_id_t& tree_target,
                 element_id_t tree_deleted) -> void {
    auto& layout = circuit.layout();

    // delete first, so caches are emptied
    const auto tree_copy = SegmentTree {layout.segment_tree(tree_deleted)};
    delete_inserted_tree_segments(circuit, sender, tree_deleted);
    swap_and_delete_single_element_private(circuit, sender, tree_deleted, &tree_target);

    // add segments to new tree
    auto&& m_tree = layout.modifyable_segment_tree(tree_target);
    auto first_index = m_tree.add_tree(tree_copy);

    for (auto segment_index : range(first_index, ++m_tree.last_index())) {
        const auto segment = segment_t {tree_target, segment_index};
        const auto segment_info = m_tree.segment_info(segment_index);

        sender.submit(info_message::SegmentCreated {segment});
        sender.submit(info_message::SegmentInserted {segment, segment_info});
    }
}

auto updated_segment_info(segment_info_t segment_info, point_t position,
                          SegmentPointType point_type) {
    if (segment_info.line.p0 == position) {
        segment_info.p0_type = point_type;
    } else if (segment_info.line.p1 == position) {
        segment_info.p1_type = point_type;
    } else {
        throw_exception("Position needs to be an endpoint of the segment.");
    }
    return segment_info;
}

auto update_segment_point_types(
    Circuit& circuit, MessageSender sender,
    std::initializer_list<const std::pair<segment_t, SegmentPointType>> data,
    point_t position) -> void {
    auto& layout = circuit.layout();

    for (auto [segment, point_type] : data) {
        if (!is_inserted(circuit, segment.element_id)) [[unlikely]] {
            throw_exception("only works for inserted wires");
        }

        auto& m_tree = layout.modifyable_segment_tree(segment.element_id);
        const auto old_segment_info = m_tree.segment_info(segment.segment_index);
        const auto new_segment_info
            = updated_segment_info(old_segment_info, position, point_type);

        m_tree.update_segment(segment.segment_index, new_segment_info);
        sender.submit(info_message::InsertedEndPointsUpdated {
            .segment = segment,
            .new_segment_info = old_segment_info,
            .old_segment_info = new_segment_info,
        });
    }
}

auto sort_through_lines_first(std::span<std::pair<ordered_line_t, segment_t>> lines,
                              point_t point) -> void {
    std::ranges::sort(lines, {}, [point](std::pair<ordered_line_t, segment_t> item) {
        return is_endpoint(point, item.first);
    });
}

auto merge_parallel_segments(segment_info_t segment_info_0, segment_info_t segment_info_1)
    -> segment_info_t {
    const auto [a, b] = order_points(segment_info_0, segment_info_1);

    if (a.line.p1 != b.line.p0) [[unlikely]] {
        throw_exception("segments need to have common shared point");
    }
    if (is_connection(a.p1_type) || is_connection(b.p0_type)) {
        throw_exception("cannot merge segments with connections");
    }

    return segment_info_t {
        .line = ordered_line_t {a.line.p0, b.line.p1},

        .p0_type = a.p0_type,
        .p1_type = b.p1_type,

        .p0_connection_id = a.p0_connection_id,
        .p1_connection_id = b.p1_connection_id,
    };
}

auto merge_line_segments(Layout& layout, MessageSender sender, segment_t segment_0,
                         segment_t segment_1) -> void {
    const auto [index0, index1]
        = sorted(segment_0.segment_index, segment_1.segment_index);
    const auto element_id = segment_0.element_id;

    if (segment_0.element_id != segment_1.element_id) [[unlikely]] {
        throw_exception("Cannot merge segments of different trees.");
    }
    if (!is_inserted(layout, element_id)) [[unlikely]] {
        throw_exception("Can only merge inserted segments.");
    }
    if (index0 == index1) [[unlikely]] {
        throw_exception("Cannot merge the same segments.");
    }

    auto& m_tree = layout.modifyable_segment_tree(element_id);
    const auto index_last = m_tree.last_index();
    const auto segment_last = segment_t {element_id, index_last};

    const auto info_0 = m_tree.segment_info(index0);
    const auto info_1 = m_tree.segment_info(index1);
    const auto info_merged = merge_parallel_segments(info_0, info_1);

    // merge
    m_tree.update_segment(index0, info_merged);
    m_tree.swap_and_delete_segment(index1);

    // messages
    sender.submit(info_message::SegmentUninserted {segment_0, info_0});
    sender.submit(info_message::SegmentUninserted {segment_1, info_1});
    sender.submit(info_message::SegmentInserted {segment_0, info_merged});

    if (index1 != index_last) {
        sender.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
        });
        sender.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
            .segment_info = m_tree.segment_info(segment_1.segment_index),
        });
    }

    // TODO SegmentPartMoved
}

auto fix_and_merge_line_segments(State state, point_t position,
                                 segment_part_t* preserve_segment) -> void {
    auto& layout = state.layout;

    // TODO rename to segments
    const auto segments = state.cache.spatial_cache().query_line_segments(position);
    const auto segment_count = get_segment_count(segments);

    if (segment_count == 0) [[unlikely]] {
        throw_exception("Could not find any segments at position.");
    }
    if (!all_same_element_id(segments)) [[unlikely]] {
        throw_exception("All segments need to belong to the same segment tree.");
    }

    if (segment_count == 1) {
        update_segment_point_types(
            state.circuit, state.sender,
            {
                std::pair {segments.at(0), SegmentPointType::output},
            },
            position);
        return;
    }

    if (segment_count == 2) {
        auto lines = std::array {
            std::pair {get_line(layout, segments.at(0)), segments.at(0)},
            std::pair {get_line(layout, segments.at(1)), segments.at(1)},
        };
        sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            const auto cross_point_type = is_horizontal(lines.at(1).first)
                                              ? SegmentPointType::cross_point_horizontal
                                              : SegmentPointType::cross_point_vertical;
            update_segment_point_types(
                state.circuit, state.sender,
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
            merge_line_segments(state.layout, state.sender, segments.at(0),
                                segments.at(1));
            return;
        }

        // handle corner
        update_segment_point_types(
            state.circuit, state.sender,
            {
                std::pair {segments.at(0), SegmentPointType::colliding_point},
                std::pair {segments.at(1), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    if (segment_count == 3) {
        auto lines = std::array {
            std::pair {get_line(layout, segments.at(0)), segments.at(0)},
            std::pair {get_line(layout, segments.at(1)), segments.at(1)},
            std::pair {get_line(layout, segments.at(2)), segments.at(2)},
        };
        sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            const auto cross_point_type = is_horizontal(lines.at(2).first)
                                              ? SegmentPointType::cross_point_horizontal
                                              : SegmentPointType::cross_point_vertical;
            update_segment_point_types(
                state.circuit, state.sender,
                {
                    std::pair {lines.at(1).second, SegmentPointType::shadow_point},
                    std::pair {lines.at(2).second, cross_point_type},
                },
                position);
        } else {
            update_segment_point_types(
                state.circuit, state.sender,
                {
                    std::pair {segments.at(0), SegmentPointType::colliding_point},
                    std::pair {segments.at(1), SegmentPointType::shadow_point},
                    std::pair {segments.at(2), SegmentPointType::visual_cross_point},
                },
                position);
        }
        return;
    }

    if (segment_count == 4) {
        update_segment_point_types(
            state.circuit, state.sender,
            {
                std::pair {segments.at(0), SegmentPointType::colliding_point},
                std::pair {segments.at(1), SegmentPointType::shadow_point},
                std::pair {segments.at(2), SegmentPointType::shadow_point},
                std::pair {segments.at(3), SegmentPointType::visual_cross_point},
            },
            position);
        return;
    }

    throw_exception("unexpected unhandeled case");
}

auto add_and_merge_segment(State state, ordered_line_t line) -> segment_part_t {
    const auto colliding_id_0 = state.cache.collision_cache().get_first_wire(line.p0);
    const auto colliding_id_1 = state.cache.collision_cache().get_first_wire(line.p1);

    auto element_id = colliding_id_0;

    if (colliding_id_1) {
        if (element_id) {
            merge_trees(state.circuit, state.sender, element_id, colliding_id_1);
        } else {
            element_id = colliding_id_1;
        }
    }

    if (!element_id) {
        element_id = add_new_temporary_wire_element(state.circuit, state.sender);
    }

    auto segment_part
        = add_segment_to_tree(state.circuit, state.sender, element_id, line);

    // now fix all endpoints at given positions
    fix_and_merge_line_segments(state, line.p0, &segment_part);
    fix_and_merge_line_segments(state, line.p1, &segment_part);

#ifndef NDEBUG
    state.layout.segment_tree(element_id).validate_inserted();
#endif

    return segment_part;
}

auto insert_wire(State state, ordered_line_t line) -> segment_part_t {
    const auto segment_part = add_and_merge_segment(state, line);
    // TODO connect segment
    return segment_part;
}

// this tree simply removes the segment from the tree, which might become empty
// the point type will revert to shadow_point
auto remove_wire_segment_from_tree(Circuit& circuit, MessageSender sender,
                                   segment_part_t& segment_part) -> void {
    // segment_part_t* preserve_part

    // TODO introduce ordered line ???

    const auto full_line = get_line(circuit.layout(), segment_part.segment);
    const auto full_part = to_part(full_line);

    const auto removing_part = segment_part.part;
    const auto removing_line = to_line(full_line, removing_part);

    auto& m_tree
        = circuit.layout().modifyable_segment_tree(segment_part.segment.element_id);

    // remove completely
    if (a_equal_b(removing_part, full_part)) {
        const auto last_segment
            = segment_t {segment_part.segment.element_id, m_tree.last_index()};
        m_tree.swap_and_delete_segment(segment_part.segment.segment_index);

        sender.submit(info_message::SegmentPartDeleted {segment_part});

        if (last_segment != segment_part.segment) {
            sender.submit(info_message::SegmentIdUpdated {
                .new_segment = segment_part.segment,
                .old_segment = last_segment,
            });

            // TODO InsertedSegmentIdUpdated
        }
    }

    // shrink one side of segment
    else if (a_inside_b_touching_one_side(removing_part, full_part)) {
        const auto keep_line = full_line.p0 == removing_line.p0
                                   ? ordered_line_t {removing_line.p1, full_line.p1}
                                   : ordered_line_t {full_line.p0, removing_line.p0};

        m_tree.update_segment(segment_part.segment.segment_index,
                              segment_info_t {.line = keep_line,
                                              .p0_type = SegmentPointType::shadow_point,
                                              .p1_type = SegmentPointType::shadow_point});
    }

    // split segment in two
    else if (a_inside_b_not_touching(removing_part, full_part)) {
        const auto keep_line = full_line.p0 == removing_line.p0
                                   ? line_t {removing_line.p1, full_line.p1}
                                   : line_t {full_line.p0, removing_line.p0};
        print(keep_line);
    }

    else {
        throw_exception("segment_part does not exist in its entire length");
    }

    segment_part = null_segment_part;
}

auto mark_valid(Layout& layout, segment_part_t segment_part) {
    auto& m_tree = layout.modifyable_segment_tree(segment_part.segment.element_id);
    m_tree.mark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto unmark_valid(Layout& layout, segment_part_t segment_part) {
    auto& m_tree = layout.modifyable_segment_tree(segment_part.segment.element_id);
    m_tree.unmark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto wire_change_temporary_to_colliding(State state, segment_part_t& segment_part)
    -> void {
    const auto line = get_line(state.layout, segment_part);
    remove_wire_segment_from_tree(state.circuit, state.sender, segment_part);

    if (is_wire_colliding(state.cache, line)) {
        segment_part = add_segment_to_aggregate(state.circuit, state.sender, line,
                                                display_state_t::new_colliding);
    } else {
        segment_part = insert_wire(state, line);
        mark_valid(state.layout, segment_part);
    }
}

auto wire_change_colliding_to_insert(Circuit& circuit, MessageSender sender,
                                     segment_part_t segment_part) -> void {}

auto wire_change_insert_to_colliding(Layout& layout, segment_part_t segment_part)
    -> void {}

auto wire_change_colliding_to_temporary(Circuit& circuit, MessageSender sender,
                                        segment_part_t& segment_part) -> void {}

auto change_wire_insertion_mode(State state, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void {
    if (!segment_part) [[unlikely]] {
        throw_exception("segment part is invalid");
    }
    if (!state.schematic.element(segment_part.segment.element_id).is_wire())
        [[unlikely]] {
        throw_exception("only works on wires");
    }

    // as parts have length, the line segment can have two possible modes
    const auto old_modes = get_insertion_modes(state.layout, segment_part);
    print(old_modes);

    if (old_modes.first == new_mode && old_modes.second == new_mode) {
        return;
    }

    if (old_modes.first == InsertionMode::temporary
        || old_modes.second == InsertionMode::temporary) {
        wire_change_temporary_to_colliding(state, segment_part);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        wire_change_colliding_to_insert(state.circuit, state.sender, segment_part);
    }
    if (old_modes.first == InsertionMode::insert_or_discard
        || old_modes.second == InsertionMode::insert_or_discard) {
        wire_change_insert_to_colliding(state.layout, segment_part);
    }
    if (new_mode == InsertionMode::temporary) {
        wire_change_colliding_to_temporary(state.circuit, state.sender, segment_part);
    }
}

// adding segments

auto add_wire_segment(State state, line_t line, InsertionMode insertion_mode)
    -> segment_part_t {
    auto segment_part
        = add_segment_to_aggregate(state.circuit, state.sender, ordered_line_t {line},
                                   display_state_t::new_temporary);
    change_wire_insertion_mode(state, segment_part, insertion_mode);
    return segment_part;
}

auto add_wire_segment(State state, Selection* selection, line_t line,
                      InsertionMode insertion_mode) -> void {
    auto segment_part = add_wire_segment(state, line, insertion_mode);

    if (selection != nullptr) {
        selection->add_segment(segment_part);
    }
}

auto add_connected_wire(State state, point_t p0, point_t p1, LineSegmentType segment_type,
                        Selection* selection) -> void {
    const auto mode = InsertionMode::temporary;

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

}  // namespace editable_circuit

}  // namespace logicsim
