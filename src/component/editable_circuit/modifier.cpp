#include "modifier.h"

#include "component/editable_circuit/editing/edit_logicitem.h"
#include "component/editable_circuit/editing/edit_wire.h"
#include "component/editable_circuit/modifier.h"
#include "format/pointer.h"
#include "logging.h"
#include "modifier.h"
#include "tree_normalization.h"

#include <fmt/core.h>

namespace logicsim {

namespace editable_circuit {

constexpr static inline auto DEBUG_PRINT_MODIFIER_METHODS = false;
constexpr static inline auto DEBUG_CHECK_CLASS_INVARIANTS = false;

namespace {

[[nodiscard]] auto debug_class_invariant_holds(const Modifier& modifier) -> bool {
    if constexpr (DEBUG_CHECK_CLASS_INVARIANTS) {
        assert(class_invariant_holds(modifier));
    }
    return true;
}

}  // namespace

//
// Modifier
//

Modifier::Modifier(ModifierConfig config) : Modifier(Layout {}, config) {}

Modifier::Modifier(Layout&& layout__)
    : Modifier {std::move(layout__), ModifierConfig {}} {}

Modifier::Modifier(Layout&& layout__, ModifierConfig config)
    : circuit_data_ {std::move(layout__)} {
    circuit_data_.store_messages = config.store_messages;

    Ensures(debug_class_invariant_holds(*this));
}

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
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "delete_temporary_logicitem(logicitem_id = {}, preserve_element = "
            "{});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, fmt_ptr(preserve_element));
    }

    editing::delete_temporary_logicitem(circuit_data_, logicitem_id, preserve_element);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_temporary_logicitem_unchecked(const logicitem_id_t logicitem_id,
                                                  int dx, int dy) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_temporary_logicitem_unchecked(logicitem_id = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, dx, dy);
    }

    editing::move_temporary_logicitem_unchecked(circuit_data_.layout, logicitem_id, dx,
                                                dy);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_or_delete_temporary_logicitem(logicitem_id_t& logicitem_id, int dx,
                                                  int dy) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_temporary_logicitem(logicitem_id = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, dx, dy);
    }

    editing::move_or_delete_temporary_logicitem(circuit_data_, logicitem_id, dx, dy);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::change_logicitem_insertion_mode(logicitem_id_t& logicitem_id,
                                               InsertionMode new_insertion_mode) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_logicitem_insertion_mode(logicitem_id = {}, new_mode = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, new_insertion_mode);
    }

    editing::change_logicitem_insertion_mode(circuit_data_, logicitem_id,
                                             new_insertion_mode);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_logicitem(const LogicItemDefinition& definition, point_t position,
                             InsertionMode insertion_mode) -> logicitem_id_t {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_logicitem(definition = {}, position = {}, insertion_mode = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, definition, position, insertion_mode);
    }

    const auto logicitem_id =
        editing::add_logicitem(circuit_data_, definition, position, insertion_mode);

    Ensures(debug_class_invariant_holds(*this));
    return logicitem_id;
}

auto Modifier::toggle_inverter(point_t point) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "toggle_inverter(point = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, point);
    }

    editing::toggle_inverter(circuit_data_, point);
}

auto Modifier::set_attributes(logicitem_id_t logicitem_id,
                              attributes_clock_generator_t attrs__) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "set_attributes(logicitem_id = {}, attrs_ = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, attrs__);
    }

    circuit_data_.layout.logic_items().set_attributes(logicitem_id, std::move(attrs__));
    Ensures(debug_class_invariant_holds(*this));
}

//
// Wires
//

auto Modifier::delete_temporary_wire_segment(segment_part_t& segment_part) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "delete_temporary_wire_segment(segment_part = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment_part);
    }

    editing::delete_temporary_wire_segment(circuit_data_, segment_part);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_wire_segment(ordered_line_t line, InsertionMode insertion_mode)
    -> segment_part_t {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_wire_segment(line = {}, new_mode = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, line, insertion_mode);
    }

    const auto segment = editing::add_wire_segment(circuit_data_, line, insertion_mode);

    Ensures(debug_class_invariant_holds(*this));
    return segment;
}

auto Modifier::change_wire_insertion_mode(segment_part_t& segment_part,
                                          InsertionMode new_insertion_mode) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_wire_insertion_mode(segment_part = {}, new_mode = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment_part, new_insertion_mode);
    }

    editing::change_wire_insertion_mode(circuit_data_, segment_part, new_insertion_mode);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_temporary_wire_unchecked(segment_t segment, part_t verify_full_part,
                                             int dx, int dy) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_temporary_wire_unchecked(segment = {}, verify_full_part = {}, "
            "dx = {}, dy = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment, verify_full_part, dx, dy);
    }

    editing::move_temporary_wire_unchecked(circuit_data_.layout, segment,
                                           verify_full_part, dx, dy);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_or_delete_temporary_wire(segment_part_t& segment_part, int dx, int dy)
    -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_temporary_wire(segment_part = {}, dx = {}, dy = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment_part, dx, dy);
    }

    editing::move_or_delete_temporary_wire(circuit_data_, segment_part, dx, dy);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::toggle_wire_crosspoint(point_t point) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "toggle_wire_crosspoint(point = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, point);
    }

    editing::toggle_wire_crosspoint(circuit_data_, point);
    Ensures(debug_class_invariant_holds(*this));
}

//
// Wire Normalization
//

auto Modifier::regularize_temporary_selection(
    const Selection& selection, std::optional<std::vector<point_t>> true_cross_points__)
    -> std::vector<point_t> {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "regularize_temporary_selection(selection = {}, true_cross_points = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, selection, true_cross_points__);
    }

    const auto points = editing::regularize_temporary_selection(
        circuit_data_, selection, std::move(true_cross_points__));

    Ensures(debug_class_invariant_holds(*this));
    return points;
}

auto Modifier::split_temporary_segments(const Selection& selection,
                                        std::span<const point_t> split_points) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "split_temporary_segments(selection = {}, split_points = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, selection, split_points);
    }

    editing::split_temporary_segments(circuit_data_, selection, split_points);
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::create_selection() -> selection_id_t {
    const auto selection_id = circuit_data_.selection_store.create();

    Ensures(debug_class_invariant_holds(*this));
    return selection_id;
}

auto Modifier::create_selection(Selection selection__) -> selection_id_t {
    // this method needs to take selection as a copy, as create might invalidate the
    // reference, if the underlying vector is resized and given selection points to it.

    if (!is_valid_selection(selection__, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    const auto selection_id = circuit_data_.selection_store.create();
    circuit_data_.selection_store.at(selection_id) = std::move(selection__);

    Ensures(debug_class_invariant_holds(*this));
    return selection_id;
}

auto Modifier::create_selection(selection_id_t copy_id) -> selection_id_t {
    const auto new_id = circuit_data_.selection_store.create();
    circuit_data_.selection_store.at(new_id) = circuit_data_.selection_store.at(copy_id);

    Ensures(debug_class_invariant_holds(*this));
    return new_id;
}

auto Modifier::destroy_selection(selection_id_t selection_id) -> void {
    circuit_data_.selection_store.destroy(selection_id);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::set_selection(selection_id_t selection_id, Selection selection__) -> void {
    if (!is_valid_selection(selection__, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    circuit_data_.selection_store.at(selection_id) = std::move(selection__);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_to_selection(selection_id_t selection_id, logicitem_id_t logicitem_id)
    -> void {
    if (!is_id_valid(logicitem_id, circuit_data_.layout)) {
        throw std::runtime_error("Logicitem id is not part of layout");
    }

    circuit_data_.selection_store.at(selection_id).add_logicitem(logicitem_id);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_to_selection(selection_id_t selection_id, segment_part_t segment_part)
    -> void {
    if (!is_segment_part_valid(segment_part, circuit_data_.layout)) {
        throw std::runtime_error("Segment part is not part of layout");
    }

    circuit_data_.selection_store.at(selection_id).add_segment(segment_part);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::remove_from_selection(selection_id_t selection_id,
                                     logicitem_id_t logicitem_id) -> void {
    circuit_data_.selection_store.at(selection_id).remove_logicitem(logicitem_id);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::remove_from_selection(selection_id_t selection_id,
                                     segment_part_t segment_part) -> void {
    circuit_data_.selection_store.at(selection_id).remove_segment(segment_part);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::clear_visible_selection() -> void {
    circuit_data_.visible_selection.clear();

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::set_visible_selection(Selection selection__) -> void {
    if (!is_valid_selection(selection__, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    circuit_data_.visible_selection.set_selection(std::move(selection__));

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_visible_selection_rect(SelectionFunction function, rect_fine_t rect)
    -> void {
    circuit_data_.visible_selection.add(function, rect);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::try_pop_last_visible_selection_rect() -> bool {
    if (circuit_data_.visible_selection.operation_count() == size_t {0}) {
        return false;
    }
    circuit_data_.visible_selection.pop_last();

    Ensures(debug_class_invariant_holds(*this));
    return true;
}

auto Modifier::try_update_last_visible_selection_rect(rect_fine_t rect) -> bool {
    if (circuit_data_.visible_selection.operation_count() == size_t {0}) {
        return false;
    }
    circuit_data_.visible_selection.update_last(rect);

    Ensures(debug_class_invariant_holds(*this));
    return true;
}

auto Modifier::apply_all_visible_selection_operations() -> void {
    circuit_data_.visible_selection.apply_all_operations(circuit_data_.layout,
                                                         circuit_data_.index);

    Ensures(debug_class_invariant_holds(*this));
}

//
// Free Methods
//

auto class_invariant_holds(const Modifier& modifier) -> bool {
    const auto& circuit = modifier.circuit_data();

    // NOT CHECKED:
    //   Inserted Logic Items:
    //      + Are not colliding with anything.
    //      + All connections with wires are compatible (type & orientation).
    //   Inserted Wires:
    //      + Segments are not colliding with anything.
    //      + Input corresponds to logicitem output and has correct orientation / position
    //      + Segments form a flat tree. With input at the root.
    //      + Have correctly set SegmentPointTypes (input, output, corner, cross, shadow).

    // Logic Item
    Expects(std::ranges::all_of(logicitem_ids(circuit.layout),
                                [](const logicitem_id_t& logicitem_id) { return true; }));

    // Inserted Wires

    // Uninserted Wires
    for (const auto wire_id : {temporary_wire_id, colliding_wire_id}) {
        const auto& segment_tree = circuit.layout.wires().segment_tree(wire_id);

        Expects(segment_tree.valid_parts().empty());
        Expects(segment_tree.input_count() == connection_count_t {0});
        Expects(segment_tree.output_count() == connection_count_t {0});
        Expects(
            std::ranges::all_of(segment_tree.segments(), [](const segment_info_t& info) {
                return info.p0_type == SegmentPointType::shadow_point &&
                       info.p1_type == SegmentPointType::shadow_point;
            }));
    }

    // Inserted Logic Items

    // Layout Index
    Expects(circuit.index == LayoutIndex {circuit.layout});

    // Selections
    const auto selection_valid = [&](const Selection& selection) {
        return is_valid_selection(selection, circuit.layout);
    };
    Expects(std::ranges::all_of(std::ranges::views::values(circuit.selection_store),
                                selection_valid));
    Expects(selection_valid(
        circuit.visible_selection.selection(circuit.layout, circuit.index)));

    return true;
}

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
    return editing::are_logicitem_positions_representable(layout, selection, delta_x,
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
