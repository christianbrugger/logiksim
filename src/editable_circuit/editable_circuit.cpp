#include "editable_circuit.h"

#include "editable_circuit/caches.h"
#include "editable_circuit/caches/split_point_cache.h"
#include "editable_circuit/handler_examples.h"
#include "editable_circuit/handlers.h"
#include "exceptions.h"
#include "scene.h"
#include "timer.h"

namespace logicsim {

//
// Editable Circuit
//

EditableCircuit::EditableCircuit(Layout&& layout)
    : layout_ {std::move(layout)},
      cache_provider_ {layout_.value()},
      selection_builder_ {layout_.value(), cache_provider_} {}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}}}", layout_);
}

auto EditableCircuit::layout() const -> const Layout& {
    return layout_.value();
}

auto EditableCircuit::extract_layout() -> Layout {
    auto temp = Layout {std::move(layout_.value())};

    // we don't reset the registrar, as allocations might still be out there
    layout_ = std::nullopt;
    cache_provider_ = CacheProvider {};
    selection_builder_ = SelectionBuilder {Layout {}, cache_provider_};

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
                return display_state_t::colliding;
            } else {
                return display_state_t::valid;
            }

        case temporary:
            return display_state_t::temporary;
    };
    throw_exception("unknown insertion mode");
}

auto EditableCircuit::validate() -> void {
    const auto& layout = layout_.value();

    layout.validate();
    cache_provider_.validate(layout);
    registrar_.validate(layout);
    selection_builder_.validate(layout);
}

auto EditableCircuit::add_example() -> void {
    auto rng = get_random_number_generator();
    editable_circuit::examples::add_many_wires_and_buttons(rng, get_state(), false);
}

auto EditableCircuit::add_logic_item(LogicItemDefinition definition, point_t position,
                                     InsertionMode insertion_mode) -> selection_handle_t {
    const auto element_id = editable_circuit::add_logic_item(get_state(), definition,
                                                             position, insertion_mode);

    auto handle = registrar_.create_selection();
    if (element_id) {
        handle.value().add_logicitem(element_id);
    }
    return handle;
}

auto EditableCircuit::add_logic_item(LogicItemDefinition definition, point_t position,
                                     InsertionMode insertion_mode,
                                     const selection_handle_t& handle) -> void {
    const auto element_id = editable_circuit::add_logic_item(get_state(), definition,
                                                             position, insertion_mode);

    if (element_id) {
        handle.value().add_logicitem(element_id);
    }
}

auto EditableCircuit::add_line_segment(line_t line, InsertionMode insertion_mode)
    -> selection_handle_t {
    auto handle = registrar_.create_selection();

    add_wire_segment(get_state(), handle.get(), line, insertion_mode);

    return handle;
}

auto EditableCircuit::add_line_segment(line_t line, InsertionMode insertion_mode,
                                       const selection_handle_t& handle) -> void {
    add_wire_segment(get_state(), handle.get(), line, insertion_mode);
}

auto EditableCircuit::add_line_segments(point_t p0, point_t p1,
                                        LineInsertionType segment_type,
                                        InsertionMode insertion_mode)
    -> selection_handle_t {
    auto handle = registrar_.create_selection();

    editable_circuit::add_wire(get_state(), p0, p1, segment_type, insertion_mode,
                               handle.get());

    return handle;
}

auto EditableCircuit::add_line_segments(point_t p0, point_t p1,
                                        LineInsertionType segment_type,
                                        InsertionMode insertion_mode,
                                        const selection_handle_t& handle) -> void {
    editable_circuit::add_wire(get_state(), p0, p1, segment_type, insertion_mode,
                               handle.get());
}

namespace {}  // namespace

auto EditableCircuit::new_positions_representable(const Selection& selection, int delta_x,
                                                  int delta_y) const -> bool {
    return editable_circuit::new_positions_representable(selection, layout_.value(),
                                                         delta_x, delta_y);
}

auto EditableCircuit::move_or_delete_elements(selection_handle_t handle, int delta_x,
                                              int delta_y) -> void {
    editable_circuit::move_or_delete_elements(std::move(handle), layout_.value(),
                                              get_sender(), delta_x, delta_y);
}

auto EditableCircuit::change_insertion_mode(selection_handle_t handle,
                                            InsertionMode new_insertion_mode) -> void {
    editable_circuit::change_insertion_mode(std::move(handle), get_state(),
                                            new_insertion_mode);
}

auto EditableCircuit::delete_all(selection_handle_t handle) -> void {
    editable_circuit::delete_all(std::move(handle), get_state());
}

auto EditableCircuit::toggle_inverter(point_t point) -> void {
    auto& layout = layout_.value();

    if (const auto entry = cache_provider_.input_cache().find(point);
        entry.has_value() && entry->is_connection()) {
        const auto element = layout.element(entry->element_id);
        element.set_input_inverter(entry->connection_id,
                                   !element.input_inverted(entry->connection_id));
    }

    if (const auto entry = cache_provider_.output_cache().find(point);
        entry.has_value() && entry->is_connection()) {
        const auto element = layout.element(entry->element_id);
        element.set_output_inverter(entry->connection_id,
                                    !element.output_inverted(entry->connection_id));
    }
}

auto EditableCircuit::toggle_wire_crosspoint(point_t point) -> void {
    editable_circuit::toggle_inserted_wire_crosspoint(get_state(), point);
}

auto EditableCircuit::regularize_temporary_selection(const Selection& selection)
    -> std::vector<point_t> {
    return editable_circuit::regularize_temporary_selection(layout_.value(), get_sender(),
                                                            selection);
}

auto EditableCircuit::capture_inserted_cross_points(const Selection& selection) const
    -> std::vector<point_t> {
    return editable_circuit::capture_inserted_cross_points(layout_.value(),
                                                           cache_provider_, selection);
}

auto EditableCircuit::split_temporary_segments(std::span<const point_t> split_points,
                                               const Selection& selection) -> void {
    editable_circuit::split_temporary_segments(layout_.value(), get_sender(),
                                               split_points, selection);
}

auto EditableCircuit::capture_new_splitpoints(const Selection& selection) const
    -> std::vector<point_t> {
    return editable_circuit::capture_new_splitpoints(layout_.value(), cache_provider_,
                                                     selection);
}

auto EditableCircuit::create_selection() const -> selection_handle_t {
    return registrar_.create_selection();
}

auto EditableCircuit::create_selection(const Selection& selection) const
    -> selection_handle_t {
    return registrar_.create_selection(selection);
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
    registrar_.submit(message);
    selection_builder_.submit(message);
}

auto EditableCircuit::get_sender() -> editable_circuit::MessageSender {
    return editable_circuit::MessageSender {*this};
}

auto EditableCircuit::get_state() -> editable_circuit::State {
    return editable_circuit::State {layout_.value(), get_sender(), cache_provider_};
}

auto move_or_delete_points(std::span<const point_t> points, int delta_x, int delta_y)
    -> std::vector<point_t> {
    auto result = std::vector<point_t> {};
    result.reserve(points.size());

    for (const auto& point : points) {
        if (is_representable(point, delta_x, delta_y)) {
            result.push_back(add_unchecked(point, delta_x, delta_y));
        }
    }

    return result;
}

}  // namespace logicsim
