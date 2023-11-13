#include "editable_circuit.h"

#include "component/editable_circuit/handler.h"
#include "format/std_type.h"
#include "geometry/point.h"
#include "layout_message.h"
#include "random/wire.h"

#include <exception>

namespace logicsim {

//
// Editable Circuit
//

EditableCircuit::EditableCircuit() : EditableCircuit {Layout {}} {}

EditableCircuit::EditableCircuit(Layout&& layout)
    : layout_ {std::move(layout)},
      layout_index_ {layout_},
      sender_ {[this](const auto& message) { this->submit(message); }},

      selection_store_ {},
      selection_builder_ {} {}

auto EditableCircuit::format() const -> std::string {
    return fmt::format("EditableCircuit{{\n{}}}", layout_);
}

auto EditableCircuit::layout() const -> const Layout& {
    return layout_;
}

auto EditableCircuit::extract_layout() -> Layout {
    auto temp = Layout {std::move(layout_)};
    *this = EditableCircuit {};
    return temp;
}

auto EditableCircuit::validate() -> void {
    layout_index_.validate(layout_);
    selection_builder_.validate(layout_, layout_index_);

    for (const auto& item : selection_store_) {
        item.second.validate(layout_);
    }
}

auto EditableCircuit::add_example() -> void {
    auto rng = get_random_number_generator();
    editable_circuit::add_many_wires_and_buttons(rng, get_state());

    // editable_circuit::add_many_wires_and_buttons(
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
                                     InsertionMode insertion_mode,
                                     selection_id_t selection_id) -> void {
    const auto logicitem_id = editable_circuit::add_logic_item(get_state(), definition,
                                                               position, insertion_mode);

    if (logicitem_id && selection_id) {
        selection(selection_id).add(logicitem_id);
    }
}

auto EditableCircuit::add_line_segment(line_t line, InsertionMode insertion_mode,
                                       selection_id_t selection_id) -> void {
    auto* selection_ptr = selection_id ? &selection(selection_id) : nullptr;
    add_wire_segment(get_state(), selection_ptr, line, insertion_mode);
}

auto EditableCircuit::add_line_segments(point_t p0, point_t p1,
                                        LineInsertionType segment_type,
                                        InsertionMode insertion_mode,
                                        selection_id_t selection_id) -> void {
    auto* selection_ptr = selection_id ? &selection(selection_id) : nullptr;
    editable_circuit::add_wire(get_state(), p0, p1, segment_type, insertion_mode,
                               selection_ptr);
}

namespace {}  // namespace

auto EditableCircuit::new_positions_representable(selection_id_t selection_id,
                                                  int delta_x, int delta_y) const
    -> bool {
    return editable_circuit::new_positions_representable(selection(selection_id), layout_,
                                                         delta_x, delta_y);
}

auto EditableCircuit::move_or_delete(selection_id_t selection_id, int delta_x,
                                     int delta_y) -> void {
    editable_circuit::move_or_delete_elements(selection(selection_id), layout_,
                                              get_sender(), delta_x, delta_y);
}

auto EditableCircuit::change_insertion_mode(selection_id_t selection_id,
                                            InsertionMode new_insertion_mode) -> void {
    editable_circuit::change_insertion_mode(selection(selection_id), get_state(),
                                            new_insertion_mode);
}

auto EditableCircuit::move_unchecked(selection_id_t selection_id, int delta_x,
                                     int delta_y) -> void {
    editable_circuit::move_unchecked(selection(selection_id), layout_, delta_x, delta_y);
}

auto EditableCircuit::delete_all(selection_id_t selection_id) -> void {
    editable_circuit::delete_all(selection(selection_id), get_state());
}

auto EditableCircuit::toggle_inverter(point_t point) -> void {
    editable_circuit::toggle_inverter(layout_, layout_index_, point);
}

auto EditableCircuit::toggle_wire_crosspoint(point_t point) -> void {
    editable_circuit::toggle_inserted_wire_crosspoint(get_state(), point);
}

auto EditableCircuit::set_attributes(logicitem_id_t logicitem_id,
                                     attributes_clock_generator_t attrs) -> void {
    layout_.logic_items().set_attributes(logicitem_id, std::move(attrs));
}

auto EditableCircuit::regularize_temporary_selection(
    selection_id_t selection_id, std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t> {
    return editable_circuit::regularize_temporary_selection(
        layout_, get_sender(), selection(selection_id), true_cross_points);
}

auto EditableCircuit::capture_inserted_cross_points(const Selection& selection) const
    -> std::vector<point_t> {
    return editable_circuit::capture_inserted_cross_points(layout_, layout_index_,
                                                           selection);
}

auto EditableCircuit::split_before_insert(selection_id_t selection_id) -> void {
    const auto split_points = editable_circuit::capture_new_splitpoints(
        layout_, layout_index_, selection(selection_id));

    editable_circuit::split_temporary_segments(
        layout_, get_sender(), std::move(split_points), selection(selection_id));
}

auto EditableCircuit::selection(selection_id_t selection_id) -> Selection& {
    return selection_store_.at(selection_id);
}

auto EditableCircuit::selection(selection_id_t selection_id) const -> const Selection& {
    return selection_store_.at(selection_id);
}

auto EditableCircuit::create_selection() -> selection_id_t {
    return selection_store_.create();
}

auto EditableCircuit::destroy_selection(selection_id_t selection_id) {
    selection_store_.destroy(selection_id);
}

auto EditableCircuit::set_visible_selection(Selection selection) -> void {
    selection_builder_.set_selection(std::move(selection));
}

auto EditableCircuit::clear_visible_selection() -> void {
    selection_builder_.clear();
}

auto EditableCircuit::add_visible_selection_rect(SelectionFunction function,
                                                 rect_fine_t rect) -> void {
    selection_builder_.add(function, rect);
}

auto EditableCircuit::update_last_visible_selection_rect(rect_fine_t rect) -> void {
    selection_builder_.update_last(rect);
}

auto EditableCircuit::apply_all_visible_selection_operations() -> void {
    selection_builder_.apply_all_operations(layout_, layout_index_);
}

auto EditableCircuit::visible_selection() const -> const Selection& {
    return selection_builder_.selection(layout_, layout_index_);
}

auto EditableCircuit::caches() const -> const LayoutIndex& {
    return layout_index_;
}

auto EditableCircuit::submit(const editable_circuit::InfoMessage& message) -> void {
    layout_index_.submit(message);
    selection_builder_.submit(message);

    for (auto& item : selection_store_) {
        item.second.submit(message);
    }
}

auto EditableCircuit::get_sender() -> editable_circuit::MessageSender& {
    return sender_;
}

auto EditableCircuit::get_state() -> editable_circuit::State {
    return editable_circuit::State {layout_, get_sender(), layout_index_};
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
