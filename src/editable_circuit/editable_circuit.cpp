#include "editable_circuit.h"

#include "editable_circuit/cache.h"
#include "editable_circuit/cache/split_point_cache.h"
#include "editable_circuit/handler.h"
#include "editable_circuit/handler_example.h"
#include "exception.h"
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
    editable_circuit::examples::add_many_wires_and_buttons(rng, get_state());

    // editable_circuit::examples::add_many_wires_and_buttons(
    //     rng, get_state(),
    //     editable_circuit::examples::WiresButtonsParams {
    //         .tries_start = 10,
    //         .tries_end = 1'000,
    //         .grid_start = 5,
    //         .grid_end = 25,
    //         .max_length = 6,
    //     });
}

auto EditableCircuit::add_logic_item(LogicItemDefinition definition, point_t position,
                                     InsertionMode insertion_mode) -> selection_handle_t {
    const auto element_id = editable_circuit::add_logic_item(get_state(), definition,
                                                             position, insertion_mode);

    auto handle = registrar_.get_handle();
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

auto EditableCircuit::get_logic_item_definition(element_id_t element_id) const
    -> LogicItemDefinition {
    return editable_circuit::get_logic_item_definition(layout_.value(), element_id);
}

auto EditableCircuit::add_line_segment(line_t line, InsertionMode insertion_mode)
    -> selection_handle_t {
    auto handle = registrar_.get_handle();

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
    auto handle = registrar_.get_handle();

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

auto EditableCircuit::move_or_delete(selection_handle_t handle, int delta_x, int delta_y)
    -> void {
    editable_circuit::move_or_delete_elements(std::move(handle), layout_.value(),
                                              get_sender(), delta_x, delta_y);
}

auto EditableCircuit::change_insertion_mode(selection_handle_t handle,
                                            InsertionMode new_insertion_mode) -> void {
    editable_circuit::change_insertion_mode(std::move(handle), get_state(),
                                            new_insertion_mode);
}

auto EditableCircuit::move_unchecked(const Selection& selection, int delta_x, int delta_y)
    -> void {
    editable_circuit::move_unchecked(selection, layout_.value(), delta_x, delta_y);
}

auto EditableCircuit::delete_all(selection_handle_t handle) -> void {
    editable_circuit::delete_all(std::move(handle), get_state());
}

auto EditableCircuit::toggle_inverter(point_t point) -> void {
    editable_circuit::toggle_inverter(layout_.value(), cache_provider_, point);
}

auto EditableCircuit::toggle_wire_crosspoint(point_t point) -> void {
    editable_circuit::toggle_inserted_wire_crosspoint(get_state(), point);
}

auto EditableCircuit::set_attributes(element_id_t element_id,
                                     layout::attributes_clock_generator attrs) -> void {
    layout_.value().set_attributes(element_id, std::move(attrs));
}

auto EditableCircuit::regularize_temporary_selection(
    const Selection& selection, std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t> {
    return editable_circuit::regularize_temporary_selection(layout_.value(), get_sender(),
                                                            selection, true_cross_points);
}

auto EditableCircuit::capture_inserted_cross_points(const Selection& selection) const
    -> std::vector<point_t> {
    return editable_circuit::capture_inserted_cross_points(layout_.value(),
                                                           cache_provider_, selection);
}

auto EditableCircuit::split_before_insert(const Selection& selection) -> void {
    const auto split_points = editable_circuit::capture_new_splitpoints(
        layout_.value(), cache_provider_, selection);

    editable_circuit::split_temporary_segments(layout_.value(), get_sender(),
                                               std::move(split_points), selection);
}

auto EditableCircuit::get_handle() const -> selection_handle_t {
    return registrar_.get_handle();
}

auto EditableCircuit::get_handle(const Selection& selection) const -> selection_handle_t {
    return registrar_.get_handle(selection);
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
    auto callback = [this](editable_circuit::InfoMessage message) {
        this->_submit(message);
    };
    return editable_circuit::MessageSender {callback};
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
