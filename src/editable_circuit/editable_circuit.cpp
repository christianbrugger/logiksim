#include "editable_circuit.h"

#include "editable_circuit/handlers.h"
#include "exceptions.h"

namespace logicsim {

//
// Editable Circuit
//

EditableCircuit::EditableCircuit(Circuit&& circuit)
    : circuit_ {std::move(circuit)},
      cache_provider_ {circuit_.value()},
      selection_builder_ {circuit_.value().layout(), cache_provider_.spatial_cache(),
                          selection_registrar_.create_selection()} {}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}}}", circuit_);
}

auto EditableCircuit::circuit() const -> const Circuit& {
    return circuit_.value();
}

auto EditableCircuit::extract_circuit() -> Circuit {
    auto temp = Circuit {std::move(circuit_.value())};

    // TODO fix reset
    circuit_ = std::nullopt;
    // selection_builder_ = SelectionBuilder {Layout {}, cache_provider_.spatial_cache(),
    //                                        selection_registrar_.create_selection()};

    return temp;
}

auto to_display_state(InsertionMode insertion_mode, bool is_colliding)
    -> display_state_t {
    switch (insertion_mode) {
        using enum InsertionMode;

        case insert_or_discard:
            return display_state_t::normal;

        case collisions:
            if (is_colliding) {
                return display_state_t::new_colliding;
            } else {
                return display_state_t::new_valid;
            }

        case temporary:
            return display_state_t::new_temporary;
    };
    throw_exception("unknown insertion mode");
}

auto EditableCircuit::validate() -> void {
    const auto& circuit = circuit_.value();

    circuit.validate();
    cache_provider_.validate(circuit);
    selection_registrar_.validate(circuit);
    selection_builder_.validate(circuit);
}

auto EditableCircuit::add_inverter_item(point_t position, InsertionMode insertion_mode,
                                        orientation_t orientation) -> selection_handle_t {
    return add_standard_logic_item(ElementType::inverter_element, 1, position,
                                   insertion_mode, orientation);
}

auto EditableCircuit::add_standard_logic_item(ElementType type, std::size_t input_count,
                                              point_t position,
                                              InsertionMode insertion_mode,
                                              orientation_t orientation)
    -> selection_handle_t {
    const auto attributes = editable_circuit::StandardLogicAttributes {
        .type = type,
        .input_count = input_count,
        .position = position,
        .orientation = orientation,
    };
    const auto element_id = editable_circuit::add_standard_logic_item(
        get_state(), attributes, insertion_mode);

    auto handle = selection_registrar_.create_selection();
    if (element_id) {
        handle.value().add_element(element_id);
    }
    return handle;
}

auto EditableCircuit::add_line_segments(point_t p0, point_t p1,
                                        LineSegmentType segment_type,
                                        InsertionMode insertion_mode)
    -> selection_handle_t {
    return selection_registrar_.create_selection();
}

namespace {

auto position_calculator(const Layout& layout, int delta_x, int delta_y) {
    return [delta_x, delta_y, &layout](element_id_t element_id) {
        const auto& element_position = layout.position(element_id);

        const int x = element_position.x.value + delta_x;
        const int y = element_position.y.value + delta_y;

        return std::make_pair(x, y);
    };
};

}  // namespace

auto EditableCircuit::new_positions_representable(const Selection& selection, int delta_x,
                                                  int delta_y) const -> bool {
    auto& circuit = circuit_.value();
    const auto get_position = position_calculator(circuit.layout(), delta_x, delta_y);

    const auto is_valid = [&](element_id_t element_id) {
        const auto [x, y] = get_position(element_id);
        return editable_circuit::is_logic_item_position_representable(circuit, element_id,
                                                                      x, y);
    };
    return std::ranges::all_of(selection.selected_elements(), is_valid);
}

auto EditableCircuit::move_or_delete_elements(selection_handle_t handle, int delta_x,
                                              int delta_y) -> void {
    if (!handle) {
        return;
    }
    const auto get_position
        = position_calculator(circuit_.value().layout(), delta_x, delta_y);

    // TODO refactor to algorithm
    while (handle->selected_elements().size() > 0) {
        auto element_id = *handle->selected_elements().begin();
        handle->remove_element(element_id);

        const auto [x, y] = get_position(element_id);
        editable_circuit::move_or_delete_logic_item(get_state(), element_id, x, y);
    }
}

auto EditableCircuit::change_insertion_mode(selection_handle_t handle,
                                            InsertionMode new_insertion_mode) -> void {
    if (!handle) {
        return;
    }

    // TODO refactor to algorithm
    while (handle->selected_elements().size() > 0) {
        auto element_id = *handle->selected_elements().begin();
        handle->remove_element(element_id);

        editable_circuit::change_logic_item_insertion_mode(get_state(), element_id,
                                                           new_insertion_mode);
    }
}

auto EditableCircuit::delete_all(selection_handle_t handle) -> void {
    if (!handle) {
        return;
    }

    // TODO refactor to algorithm
    while (handle->selected_elements().size() > 0) {
        auto element_id = *handle->selected_elements().begin();
        handle->remove_element(element_id);

        editable_circuit::change_logic_item_insertion_mode(get_state(), element_id,
                                                           InsertionMode::temporary);
        editable_circuit::swap_and_delete_single_element(circuit_.value(), get_sender(),
                                                         element_id);
    }
}

auto EditableCircuit::create_selection() const -> selection_handle_t {
    return selection_registrar_.create_selection();
}

auto EditableCircuit::create_selection(const Selection& selection) const
    -> selection_handle_t {
    return selection_registrar_.create_selection(selection);
}

auto EditableCircuit::selection_builder() const noexcept -> const SelectionBuilder& {
    return selection_builder_;
}

auto EditableCircuit::selection_builder() noexcept -> SelectionBuilder& {
    return selection_builder_;
}

auto EditableCircuit::caches() const -> const CacheProvider& {
    return cache_provider_;
}

auto EditableCircuit::_submit(editable_circuit::InfoMessage message) -> void {
    cache_provider_.submit(message);
    selection_registrar_.submit(message);
    selection_builder_.submit(message);
}

auto EditableCircuit::get_sender() -> editable_circuit::MessageSender {
    return editable_circuit::MessageSender {*this};
}

auto EditableCircuit::get_state() -> editable_circuit::State {
    auto& circuit = circuit_.value();

    return editable_circuit::State {circuit, get_sender(), cache_provider_,
                                    circuit.schematic(), circuit.layout()};
}

}  // namespace logicsim
