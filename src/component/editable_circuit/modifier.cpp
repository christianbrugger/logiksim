#include "modifier.h"

#include "component/editable_circuit/editing/edit_logicitem.h"
#include "component/editable_circuit/editing/edit_wire.h"
#include "component/editable_circuit/modifier.h"

#include <fmt/core.h>

namespace logicsim {

namespace editable_circuit {

Modifier::Modifier(Layout&& layout__) : circuit_data_ {std::move(layout__)} {}

auto Modifier::format() const -> std::string {
    return fmt::format("Modifier-{}", circuit_data_);
}

auto Modifier::circuit_data() const -> const CircuitData& {
    return circuit_data_;
}

auto Modifier::extract_layout() -> Layout {
    const auto layout = Layout {std::move(circuit_data_.layout)};
    *this = Modifier {};
    return layout;
}

//
// Logic Items
//

auto Modifier::delete_temporary_logicitem(logicitem_id_t& logicitem_id,
                                          logicitem_id_t* preserve_element) -> void {
    editing::delete_temporary_logicitem(circuit_data_, logicitem_id, preserve_element);
}

auto Modifier::move_temporary_logicitem_unchecked(const logicitem_id_t logicitem_id,
                                                  int dx, int dy) -> void {
    editing::move_temporary_logicitem_unchecked(circuit_data_.layout, logicitem_id, dx,
                                                dy);
}

auto Modifier::move_or_delete_temporary_logicitem(logicitem_id_t& logicitem_id, int dx,
                                                  int dy) -> void {
    editing::move_or_delete_temporary_logicitem(circuit_data_, logicitem_id, dx, dy);
}

auto Modifier::change_logicitem_insertion_mode(logicitem_id_t& logicitem_id,
                                               InsertionMode new_insertion_mode) -> void {
    editing::change_logicitem_insertion_mode(circuit_data_, logicitem_id,
                                             new_insertion_mode);
}

auto Modifier::add_logicitem(const LogicItemDefinition& definition, point_t position,
                             InsertionMode insertion_mode) -> logicitem_id_t {
    return editing::add_logicitem(circuit_data_, definition, position, insertion_mode);
}

auto Modifier::toggle_inverter(point_t point) -> void {
    editing::toggle_inverter(circuit_data_, point);
}

auto Modifier::set_attributes(logicitem_id_t logicitem_id,
                              attributes_clock_generator_t attrs__) -> void {
    circuit_data_.layout.logic_items().set_attributes(logicitem_id, std::move(attrs__));
}

//
// Wires
//

auto Modifier::delete_temporary_wire_segment(segment_part_t& segment_part) -> void {
    editing::delete_temporary_wire_segment(circuit_data_, segment_part);
}

auto Modifier::add_wire_segment(ordered_line_t line, InsertionMode insertion_mode)
    -> segment_part_t {
    return editing::add_wire_segment(circuit_data_, line, insertion_mode);
}

auto Modifier::change_wire_insertion_mode(segment_part_t& segment_part,
                                          InsertionMode new_insertion_mode) -> void {
    editing::change_wire_insertion_mode(circuit_data_, segment_part, new_insertion_mode);
}

auto Modifier::move_temporary_wire_unchecked(segment_t segment, part_t verify_full_part,
                                             int dx, int dy) -> void {
    editing::move_temporary_wire_unchecked(circuit_data_.layout, segment,
                                           verify_full_part, dx, dy);
}

auto Modifier::move_or_delete_temporary_wire(segment_part_t& segment_part, int dx, int dy)
    -> void {
    editing::move_or_delete_temporary_wire(circuit_data_, segment_part, dx, dy);
}

auto Modifier::toggle_wire_crosspoint(point_t point) -> void {
    editing::toggle_wire_crosspoint(circuit_data_, point);
}

//
// Wire Normalization
//

auto Modifier::regularize_temporary_selection(
    const Selection& selection, std::optional<std::vector<point_t>> true_cross_points__)
    -> std::vector<point_t> {
    return editing::regularize_temporary_selection(circuit_data_, selection,
                                                   std::move(true_cross_points__));
}

auto Modifier::split_temporary_segments(const Selection& selection,
                                        std::span<const point_t> split_points) -> void {
    editing::split_temporary_segments(circuit_data_, selection, split_points);
}

auto Modifier::create_selection() -> selection_id_t {
    return circuit_data_.selection_store.create();
}

auto Modifier::create_selection(Selection selection__) -> selection_id_t {
    // this method needs to take selection as a copy, as create might invalidate the
    // reference, if the underlying vector is resized and given selection points to it.

    if (!is_valid_selection(selection__, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    const auto selection_id = circuit_data_.selection_store.create();
    circuit_data_.selection_store.at(selection_id) = std::move(selection__);
    return selection_id;
}

auto Modifier::create_selection(selection_id_t copy_id) -> selection_id_t {
    const auto new_id = circuit_data_.selection_store.create();
    circuit_data_.selection_store.at(new_id) = circuit_data_.selection_store.at(copy_id);
    return new_id;
}

auto Modifier::destroy_selection(selection_id_t selection_id) -> void {
    circuit_data_.selection_store.destroy(selection_id);
}

auto Modifier::set_selection(selection_id_t selection_id, Selection selection__) -> void {
    if (!is_valid_selection(selection__, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    circuit_data_.selection_store.at(selection_id) = std::move(selection__);
}

auto Modifier::add_to_selection(selection_id_t selection_id, logicitem_id_t logicitem_id)
    -> void {
    if (!is_id_valid(logicitem_id, circuit_data_.layout)) {
        throw std::runtime_error("Logicitem id is not part of layout");
    }

    circuit_data_.selection_store.at(selection_id).add_logicitem(logicitem_id);
}

auto Modifier::add_to_selection(selection_id_t selection_id, segment_part_t segment_part)
    -> void {
    if (!is_segment_part_valid(segment_part, circuit_data_.layout)) {
        throw std::runtime_error("Segment part is not part of layout");
    }

    circuit_data_.selection_store.at(selection_id).add_segment(segment_part);
}

auto Modifier::remove_from_selection(selection_id_t selection_id,
                                     logicitem_id_t logicitem_id) -> void {
    circuit_data_.selection_store.at(selection_id).remove_logicitem(logicitem_id);
}

auto Modifier::remove_from_selection(selection_id_t selection_id,
                                     segment_part_t segment_part) -> void {
    circuit_data_.selection_store.at(selection_id).remove_segment(segment_part);
}

auto Modifier::clear_visible_selection() -> void {
    circuit_data_.visible_selection.clear();
}

auto Modifier::set_visible_selection(Selection selection__) -> void {
    if (!is_valid_selection(selection__, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    circuit_data_.visible_selection.set_selection(std::move(selection__));
}

auto Modifier::add_visible_selection_rect(SelectionFunction function, rect_fine_t rect)
    -> void {
    circuit_data_.visible_selection.add(function, rect);
}

auto Modifier::try_pop_last_visible_selection_rect() -> bool {
    if (circuit_data_.visible_selection.operation_count() == size_t {0}) {
        return false;
    }
    circuit_data_.visible_selection.pop_last();
    return true;
}

auto Modifier::try_update_last_visible_selection_rect(rect_fine_t rect) -> bool {
    if (circuit_data_.visible_selection.operation_count() == size_t {0}) {
        return false;
    }
    circuit_data_.visible_selection.update_last(rect);
    return true;
}

auto Modifier::apply_all_visible_selection_operations() -> void {
    circuit_data_.visible_selection.apply_all_operations(circuit_data_.layout,
                                                         circuit_data_.index);
}

//
// Free Methods
//

auto get_inserted_cross_points(const Modifier& modifier, const Selection& selection)
    -> std::vector<point_t> {
    return editing::get_inserted_cross_points(modifier.circuit_data(), selection);
}

auto get_temporary_selection_splitpoints(const Modifier& modifier,
                                         const Selection& selection)
    -> std::vector<point_t> {
    return editing::get_temporary_selection_splitpoints(modifier.circuit_data(),
                                                        selection);
}

//
// Selection Based
//

namespace {

[[nodiscard]] auto has_logicitem(const Modifier& modifier, selection_id_t selection_id)
    -> bool {
    return !modifier.circuit_data()
                .selection_store.at(selection_id)
                .selected_logic_items()
                .empty();
}

[[nodiscard]] auto get_first_logicitem(const Selection& selection) -> logicitem_id_t {
    Expects(!selection.selected_logic_items().empty());
    return selection.selected_logic_items().front();
}

[[nodiscard]] auto get_first_logicitem(const Modifier& modifier,
                                       selection_id_t selection_id) -> logicitem_id_t {
    return get_first_logicitem(modifier.circuit_data().selection_store.at(selection_id));
}

[[nodiscard]] auto has_segment(const Modifier& modifier, selection_id_t selection_id)
    -> bool {
    return !modifier.circuit_data()
                .selection_store.at(selection_id)
                .selected_segments()
                .empty();
}

[[nodiscard]] auto get_first_segment(const Selection& selection) -> segment_part_t {
    Expects(!selection.selected_segments().empty());

    return segment_part_t {
        .segment = selection.selected_segments().front().first,
        .part = selection.selected_segments().front().second.front(),
    };
}

[[nodiscard]] auto get_first_segment(const Modifier& modifier,
                                     selection_id_t selection_id) -> segment_part_t {
    return get_first_segment(modifier.circuit_data().selection_store.at(selection_id));
}

}  // namespace

auto change_insertion_mode_consuming(Modifier& modifier, selection_id_t selection_id,
                                     InsertionMode new_insertion_mode) -> void {
    // TODO store selection performance difference ?

    while (has_logicitem(modifier, selection_id)) {
        auto logicitem_id = get_first_logicitem(modifier, selection_id);
        modifier.remove_from_selection(selection_id, logicitem_id);

        modifier.change_logicitem_insertion_mode(logicitem_id, new_insertion_mode);
    }

    while (has_segment(modifier, selection_id)) {
        auto segment_part = get_first_segment(modifier, selection_id);
        modifier.remove_from_selection(selection_id, segment_part);

        modifier.change_wire_insertion_mode(segment_part, new_insertion_mode);
    }
}

auto new_positions_representable(const Layout& layout, const Selection& selection,
                                 int delta_x, int delta_y) -> bool {
    return editing::new_logicitem_positions_representable(layout, selection, delta_x,
                                                          delta_y) &&
           editing::new_wire_positions_representable(layout, selection, delta_x, delta_y);
}

auto move_temporary_unchecked(Modifier& modifier, const Selection& selection, int delta_x,
                              int delta_y) -> void {
    for (const auto& logicitem_id : selection.selected_logic_items()) {
        // TODO move checks to low-level method
        if (modifier.circuit_data().layout.logic_items().display_state(logicitem_id) !=
            display_state_t::temporary) [[unlikely]] {
            throw std::runtime_error("selected logic items need to be temporary");
        }

        modifier.move_temporary_logicitem_unchecked(logicitem_id, delta_x, delta_y);
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        // TODO move checks to low-level method
        if (parts.size() != 1) [[unlikely]] {
            throw std::runtime_error("Method assumes segments are fully selected");
        }
        if (!is_temporary(segment.wire_id)) [[unlikely]] {
            throw std::runtime_error("selected wires need to be temporary");
        }

        modifier.move_temporary_wire_unchecked(segment, parts.front(), delta_x, delta_y);
    }
}

auto move_or_delete_temporary_consuming(Modifier& modifier, selection_id_t selection_id,
                                        int delta_x, int delta_y) -> void {
    while (has_logicitem(modifier, selection_id)) {
        auto logicitem_id = get_first_logicitem(modifier, selection_id);
        modifier.remove_from_selection(selection_id, logicitem_id);

        modifier.move_or_delete_temporary_logicitem(logicitem_id, delta_x, delta_y);
    }

    while (has_segment(modifier, selection_id)) {
        auto segment_part = get_first_segment(modifier, selection_id);
        modifier.remove_from_selection(selection_id, segment_part);

        modifier.move_or_delete_temporary_wire(segment_part, delta_x, delta_y);
    }
}

auto delete_all(Modifier& modifier, selection_id_t selection_id) -> void {
    while (has_logicitem(modifier, selection_id)) {
        auto logicitem_id = get_first_logicitem(modifier, selection_id);
        modifier.remove_from_selection(selection_id, logicitem_id);

        modifier.change_logicitem_insertion_mode(logicitem_id, InsertionMode::temporary);
        modifier.delete_temporary_logicitem(logicitem_id);
    }

    while (has_segment(modifier, selection_id)) {
        auto segment_part = get_first_segment(modifier, selection_id);
        modifier.remove_from_selection(selection_id, segment_part);

        modifier.change_wire_insertion_mode(segment_part, InsertionMode::temporary);
        modifier.delete_temporary_wire_segment(segment_part);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
