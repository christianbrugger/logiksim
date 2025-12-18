#include "core/component/editable_circuit/modifier.h"

#include "core/component/editable_circuit/editing/edit_decoration.h"
#include "core/component/editable_circuit/editing/edit_history.h"
#include "core/component/editable_circuit/editing/edit_logicitem.h"
#include "core/component/editable_circuit/editing/edit_visible_selection.h"
#include "core/component/editable_circuit/editing/edit_wire.h"
#include "core/component/editable_circuit/modifier.h"
#include "core/format/pointer.h"
#include "core/logging.h"
#include "core/tree_normalization.h"

#include <fmt/core.h>

namespace logicsim {

namespace editable_circuit {

constexpr static inline auto DEBUG_PRINT_MODIFIER_METHODS = false;
constexpr static inline auto DEBUG_PRINT_CIRCUIT_HISTORY = false;
constexpr static inline auto DEBUG_CHECK_CLASS_INVARIANTS = false;

namespace {

[[nodiscard]] auto debug_class_invariant_holds(const Modifier& modifier) -> bool {
    if constexpr (DEBUG_CHECK_CLASS_INVARIANTS) {
        return is_valid(modifier);
    }
    return true;
}

}  // namespace

//
// Modifier
//

namespace {

auto create_circuit_data(Layout&& layout_, ModifierConfig config) -> CircuitData {
    auto index_ = LayoutIndex {layout_};

    auto messages_ = config.store_messages
                         ? std::optional<message_vector_t> {message_vector_t {}}
                         : std::nullopt;

    auto validator_ = config.validate_messages ? std::optional<MessageValidator> {layout_}
                                               : std::nullopt;

    auto history_ = History {};
    if (config.enable_history) {
        editing::enable_history(history_);
    }

    return CircuitData {
        .layout = std::move(layout_),
        .index = std::move(index_),
        .selection_store {},
        .visible_selection {},
        .history = std::move(history_),
        .messages = std::move(messages_),
        .message_validator = std::move(validator_),
    };
}

}  // namespace

Modifier::Modifier() : Modifier {Layout {}, ModifierConfig {}} {}

Modifier::Modifier(Layout&& layout_, ModifierConfig config)
    : circuit_data_ {create_circuit_data(std::move(layout_), config)} {
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

    Ensures(debug_class_invariant_holds(*this));
    return layout;
}

//
// Undo & Redo
//
auto Modifier::enable_history() -> void {
    editing::enable_history(circuit_data_.history);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::disable_history() -> void {
    editing::disable_history(circuit_data_.history);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::undo_group() -> void {
    editing::undo_group(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::redo_group() -> void {
    editing::redo_group(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::clear_undo_history() -> void {
    editing::clear_undo_history(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::clear_redo_history() -> void {
    editing::clear_redo_history(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::finish_undo_group() -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n"
            "finish_undo_group();\n"
            "==========================================================\n\n");
    }

    editing::finish_undo_group(circuit_data_.history);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::reopen_undo_group() -> void {
    editing::reopen_undo_group(circuit_data_.history);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

//
// Logic Items
//

auto Modifier::delete_temporary_logicitem(logicitem_id_t& logicitem_id) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "delete_temporary_logicitem(logicitem_id = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id);
    }

    editing::delete_temporary_logicitem(circuit_data_, logicitem_id);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_temporary_logicitem_unchecked(const logicitem_id_t logicitem_id,
                                                  move_delta_t delta) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_temporary_logicitem_unchecked(logicitem_id = {}, delta = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, delta);
    }

    editing::move_temporary_logicitem_unchecked(circuit_data_, logicitem_id, delta);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_or_delete_temporary_logicitem(logicitem_id_t& logicitem_id,
                                                  move_delta_t delta) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_temporary_logicitem(logicitem_id = {}, delta = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, delta);
    }

    editing::move_or_delete_temporary_logicitem(circuit_data_, logicitem_id, delta);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::change_logicitem_insertion_mode(logicitem_id_t& logicitem_id,
                                               InsertionMode new_insertion_mode,
                                               InsertionHint hint) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_logicitem_insertion_mode(logicitem_id = {}, new_mode = {}, "
            "hint = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, new_insertion_mode, hint);
    }

    editing::change_logicitem_insertion_mode(circuit_data_, logicitem_id,
                                             new_insertion_mode, hint);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_logicitem(LogicItemDefinition&& definition, point_t position,
                             InsertionMode insertion_mode) -> logicitem_id_t {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_logicitem(definition = {}, position = {}, insertion_mode = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, definition, position, insertion_mode);
    }

    const auto logicitem_id = editing::add_logicitem(circuit_data_, std::move(definition),
                                                     position, insertion_mode);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
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

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
}

auto Modifier::set_attributes(logicitem_id_t logicitem_id,
                              attributes_clock_generator_t&& attrs_) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "set_attributes(logicitem_id = {}, attrs_ = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, logicitem_id, attrs_);
    }

    editing::set_attributes_logicitem(circuit_data_, logicitem_id, std::move(attrs_));

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

//
// Decorations
//

auto Modifier::delete_temporary_decoration(decoration_id_t& decoration_id) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "delete_temporary_decoration(decoration_id = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, decoration_id);
    }

    editing::delete_temporary_decoration(circuit_data_, decoration_id);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_temporary_decoration_unchecked(const decoration_id_t decoration_id,
                                                   move_delta_t delta) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_temporary_decoration_unchecked(decoration_id = {}, delta = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, decoration_id, delta);
    }

    editing::move_temporary_decoration_unchecked(circuit_data_, decoration_id, delta);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_or_delete_temporary_decoration(decoration_id_t& decoration_id,
                                                   move_delta_t delta) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_temporary_decoration(decoration_id = {}, delta = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, decoration_id, delta);
    }

    editing::move_or_delete_temporary_decoration(circuit_data_, decoration_id, delta);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::change_decoration_insertion_mode(decoration_id_t& decoration_id,
                                                InsertionMode new_insertion_mode,
                                                InsertionHint hint) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_decoration_insertion_mode(decoration_id = {}, new_mode = {}, "
            "hint = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, decoration_id, new_insertion_mode, hint);
    }

    editing::change_decoration_insertion_mode(circuit_data_, decoration_id,
                                              new_insertion_mode, hint);
    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_decoration(DecorationDefinition&& definition, point_t position,
                              InsertionMode insertion_mode) -> decoration_id_t {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_decoration(definition = {}, position = {}, insertion_mode = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, definition, position, insertion_mode);
    }

    const auto decoration_id = editing::add_decoration(
        circuit_data_, std::move(definition), position, insertion_mode);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
    return decoration_id;
}

auto Modifier::set_attributes(decoration_id_t decoration_id,
                              attributes_text_element_t&& attrs_) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "set_attributes(decoration_id = {}, attrs_ = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, decoration_id, attrs_);
    }

    editing::set_attributes_decoration(circuit_data_, decoration_id, std::move(attrs_));

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
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

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
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

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
    return segment;
}

auto Modifier::change_wire_insertion_mode(segment_part_t& segment_part,
                                          InsertionMode new_mode, InsertionHint hint)
    -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "change_wire_insertion_mode(segment_part = {}, new_mode = {}, hint = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment_part, new_mode, hint);
    }

    editing::change_wire_insertion_mode(circuit_data_, segment_part, new_mode, hint);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_temporary_wire_unchecked(segment_part_t full_segment_part,
                                             move_delta_t delta) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_temporary_wire_unchecked(full_segment_part = {},  delta = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, full_segment_part, delta);
    }

    editing::move_temporary_wire_unchecked(circuit_data_, full_segment_part, delta);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::move_or_delete_temporary_wire(segment_part_t& segment_part,
                                             move_delta_t delta) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "move_or_delete_temporary_wire(segment_part = {}, delta = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment_part, delta);
    }

    editing::move_or_delete_temporary_wire(circuit_data_, segment_part, delta);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
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

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

//
// Wire Normalization
//

auto Modifier::set_temporary_endpoints(segment_t segment, endpoints_t endpoints) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "set_temporary_endpoints(segment = {}, endpoints = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment, endpoints);
    }

    editing::set_temporary_endpoints_with_history(circuit_data_, segment, endpoints);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::merge_uninserted_segment(segment_t segment_0, segment_t segment_1)
    -> segment_t {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "merge_uninserted_segment(segment_0 = {}, segment_1 = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment_0, segment_1);
    }

    const auto segment_merged = editing::merge_uninserted_segment_with_history(
        circuit_data_, segment_0, segment_1);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
    return segment_merged;
}

auto Modifier::split_uninserted_segment(segment_t segment, offset_t offset,
                                        segment_key_t optional_new_key)
    -> std::pair<segment_t, segment_t> {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "merge_uninserted_segment(segment = {}, offset = {}, "
            "optional_new_key = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, segment, offset, optional_new_key);
    }

    const auto result = editing::split_uninserted_segment_with_history(
        circuit_data_, segment, offset, optional_new_key);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
    return result;
}

auto Modifier::regularize_temporary_selection(
    const Selection& selection, std::optional<std::vector<point_t>> true_cross_points_)
    -> std::vector<point_t> {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "regularize_temporary_selection(selection = {}, true_cross_points = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, selection, true_cross_points_);
    }

    const auto points = editing::regularize_temporary_selection(
        circuit_data_, selection, std::move(true_cross_points_));

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
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

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

//
// Selections
//

auto Modifier::create_selection() -> selection_id_t {
    const auto selection_id = circuit_data_.selection_store.create();

    Ensures(debug_class_invariant_holds(*this));
    return selection_id;
}

auto Modifier::create_selection(Selection selection_) -> selection_id_t {
    // this method needs to take selection as a copy, as create might invalidate the
    // reference, if the underlying vector is resized and given selection points to
    // it.

    if (!is_valid_selection(selection_, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    const auto selection_id = circuit_data_.selection_store.create();
    circuit_data_.selection_store.at(selection_id) = std::move(selection_);

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

auto Modifier::set_selection(selection_id_t selection_id, Selection selection_) -> void {
    if (!is_valid_selection(selection_, circuit_data_.layout)) {
        throw std::runtime_error("Selection contains elements not in layout");
    }

    circuit_data_.selection_store.at(selection_id) = std::move(selection_);

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

auto Modifier::add_to_selection(selection_id_t selection_id,
                                decoration_id_t decoration_id) -> void {
    if (!is_id_valid(decoration_id, circuit_data_.layout)) {
        throw std::runtime_error("Decoration id is not part of layout");
    }

    circuit_data_.selection_store.at(selection_id).add_decoration(decoration_id);

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
                                     decoration_id_t decoration_id) -> void {
    circuit_data_.selection_store.at(selection_id).remove_decoration(decoration_id);

    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::remove_from_selection(selection_id_t selection_id,
                                     segment_part_t segment_part) -> void {
    circuit_data_.selection_store.at(selection_id).remove_segment(segment_part);

    Ensures(debug_class_invariant_holds(*this));
}

//
// Visible Selection
//

auto Modifier::clear_visible_selection() -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "clear_visible_selection();\n"
            "==========================================================\n\n",
            circuit_data_.layout);
    }

    editing::clear_visible_selection(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::set_visible_selection(Selection selection_) -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "set_visible_selection(selection = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, selection_);
    }

    editing::set_visible_selection(circuit_data_, std::move(selection_));

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::add_visible_selection_rect(SelectionFunction function, rect_fine_t rect)
    -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "add_visible_selection_rect(function = {}, rect = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, function, rect);
    }

    editing::add_visible_selection_rect(circuit_data_, function, rect);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

auto Modifier::try_pop_last_visible_selection_rect() -> bool {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "try_pop_last_visible_selection_rect();\n"
            "==========================================================\n\n",
            circuit_data_.layout);
    }

    if (circuit_data_.visible_selection.operations().empty()) {
        return false;
    }
    editing::pop_last_visible_selection_rect(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
    return true;
}

auto Modifier::try_update_last_visible_selection_rect(rect_fine_t rect) -> bool {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "try_update_last_visible_selection_rect(rect = {});\n"
            "==========================================================\n\n",
            circuit_data_.layout, rect);
    }

    if (circuit_data_.visible_selection.operations().empty()) {
        return false;
    }
    editing::update_last_visible_selection_rect(circuit_data_, rect);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
    return true;
}

auto Modifier::apply_all_visible_selection_operations() -> void {
    if constexpr (DEBUG_PRINT_MODIFIER_METHODS) {
        print_fmt(
            "\n==========================================================\n{}\n"
            "apply_all_visible_selection_operations();\n"
            "==========================================================\n\n",
            circuit_data_.layout);
    }

    editing::apply_all_visible_selection_operations(circuit_data_);

    if constexpr (DEBUG_PRINT_CIRCUIT_HISTORY) {
        print(circuit_data_.history);
    }
    Ensures(debug_class_invariant_holds(*this));
}

//
// Free Methods
//

namespace {

[[nodiscard]] auto inserted_wires_are_contiguous_tree_with_correct_endpoints(
    const Layout& layout) {
    const auto contiguous = [&](const wire_id_t& wire_id) {
        return is_contiguous_tree_with_correct_endpoints(
            layout.wires().segment_tree(wire_id));
    };

    return std::ranges::all_of(inserted_wire_ids(layout), contiguous);
}

[[nodiscard]] auto _temporary_point_types_valid(const Layout& layout) -> bool {
    const auto& segment_tree = layout.wires().segment_tree(temporary_wire_id);

    const auto is_valid = [](SegmentPointType type) -> bool {
        return type == SegmentPointType::shadow_point ||
               type == SegmentPointType::cross_point;
    };

    return std::ranges::all_of(segment_tree.segments(), [&](const segment_info_t& info) {
        return is_valid(info.p0_type) && is_valid(info.p1_type);
    });
}

[[nodiscard]] auto _colliding_point_types_valid(const Layout& layout) -> bool {
    const auto& segment_tree = layout.wires().segment_tree(colliding_wire_id);

    const auto is_valid = [](SegmentPointType type) -> bool {
        return type == SegmentPointType::shadow_point;
    };

    return std::ranges::all_of(segment_tree.segments(), [&](const segment_info_t& info) {
        return is_valid(info.p0_type) && is_valid(info.p1_type);
    });
}

}  // namespace

auto is_valid(const Modifier& modifier) -> bool {
    const auto& circuit = modifier.circuit_data();

    // NOT CHECKED:
    //   Logic Items
    //      + Body is fully representable within the grid.
    //   Inserted Logic Items:
    //      + Are not colliding with anything.
    //      + All connections with wires are compatible (type & orientation).
    //   Inserted Wires:
    //      + Segments are not colliding with anything.
    //      + Input corresponds to logicitem output and has correct orientation /
    //      position

    // Inserted Wires
    Expects(inserted_wires_are_contiguous_tree_with_correct_endpoints(circuit.layout));

    // Uninserted Wires
    for (const auto wire_id : {temporary_wire_id, colliding_wire_id}) {
        const auto& segment_tree = circuit.layout.wires().segment_tree(wire_id);

        Expects(!has_valid_parts(segment_tree));
        Expects(segment_tree.input_count() == connection_count_t {0});
        Expects(segment_tree.output_count() == connection_count_t {0});
    }
    Expects(_temporary_point_types_valid(circuit.layout));
    Expects(_colliding_point_types_valid(circuit.layout));

    // Layout Index
    Expects(circuit.index == LayoutIndex(circuit.layout, circuit.index.key_index()));
    Expects(circuit.index.key_index().has_all_ids_inserted(circuit.layout));

    // Selections
    const auto selection_valid = [&](const Selection& selection) {
        return is_valid_selection(selection, circuit.layout);
    };
    Expects(std::ranges::all_of(std::ranges::views::values(circuit.selection_store),
                                selection_valid));
    Expects(selection_valid(
        circuit.visible_selection.selection(circuit.layout, circuit.index)));

    // Layout Validator
    if (circuit.message_validator) {
        Expects(circuit.message_validator->layout_matches_state(circuit.layout));
    }

    return true;
}

auto get_config(const Modifier& modifier) -> ModifierConfig {
    return ModifierConfig {
        .store_messages = modifier.circuit_data().messages.has_value(),
        .validate_messages = modifier.circuit_data().message_validator.has_value(),
    };
}

auto change_wire_insertion_mode_requires_sanitization(wire_id_t wire_id,
                                                      InsertionMode new_mode) -> bool {
    return editing::change_wire_insertion_mode_requires_sanitization(wire_id, new_mode);
}

auto change_wire_insertion_mode_requires_sanitization(segment_t segment,
                                                      InsertionMode new_mode) -> bool {
    return editing::change_wire_insertion_mode_requires_sanitization(segment.wire_id,
                                                                     new_mode);
}

auto change_wire_insertion_mode_requires_sanitization(segment_part_t segment_part,
                                                      InsertionMode new_mode) -> bool {
    return editing::change_wire_insertion_mode_requires_sanitization(
        segment_part.segment.wire_id, new_mode);
}

auto are_uninserted_segments_mergeable(const Modifier& modifier, segment_t segment_0,
                                       segment_t segment_1) -> bool {
    return editing::are_uninserted_segments_mergeable(modifier.circuit_data().layout,
                                                      segment_0, segment_1);
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
                .selected_logicitems()
                .empty();
}

[[nodiscard]] auto has_decoration(const Modifier& modifier, selection_id_t selection_id)
    -> bool {
    return !modifier.circuit_data()
                .selection_store.at(selection_id)
                .selected_decorations()
                .empty();
}

[[nodiscard]] auto get_first_logicitem(const Selection& selection) -> logicitem_id_t {
    Expects(!selection.selected_logicitems().empty());
    return selection.selected_logicitems().front();
}

[[nodiscard]] auto get_first_logicitem(const Modifier& modifier,
                                       selection_id_t selection_id) -> logicitem_id_t {
    return get_first_logicitem(modifier.circuit_data().selection_store.at(selection_id));
}

[[nodiscard]] auto get_first_decoration(const Selection& selection) -> decoration_id_t {
    Expects(!selection.selected_decorations().empty());
    return selection.selected_decorations().front();
}

[[nodiscard]] auto get_first_decoration(const Modifier& modifier,
                                        selection_id_t selection_id) -> decoration_id_t {
    return get_first_decoration(modifier.circuit_data().selection_store.at(selection_id));
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

    while (has_decoration(modifier, selection_id)) {
        auto decoration_id = get_first_decoration(modifier, selection_id);
        modifier.remove_from_selection(selection_id, decoration_id);

        modifier.change_decoration_insertion_mode(decoration_id, new_insertion_mode);
    }

    while (has_segment(modifier, selection_id)) {
        auto segment_part = get_first_segment(modifier, selection_id);
        modifier.remove_from_selection(selection_id, segment_part);

        modifier.change_wire_insertion_mode(segment_part, new_insertion_mode);
    }
}

auto new_positions_representable(const Layout& layout, const Selection& selection,
                                 move_delta_t delta) -> bool {
    return editing::are_logicitem_positions_representable(layout, selection, delta) &&
           editing::are_decoration_positions_representable(layout, selection, delta) &&
           editing::new_wire_positions_representable(layout, selection, delta);
}

auto move_temporary_unchecked(Modifier& modifier, const Selection& selection,
                              move_delta_t delta) -> void {
    if (delta == move_delta_t {0, 0}) {
        return;
    }

    for (const auto& logicitem_id : selection.selected_logicitems()) {
        modifier.move_temporary_logicitem_unchecked(logicitem_id, delta);
    }

    for (const auto& decoration_id : selection.selected_decorations()) {
        modifier.move_temporary_decoration_unchecked(decoration_id, delta);
    }

    for (const auto& [segment, parts] : selection.selected_segments()) {
        Expects(!parts.empty());
        const auto segment_part = segment_part_t {segment, parts.front()};
        modifier.move_temporary_wire_unchecked(segment_part, delta);
    }
}

auto move_or_delete_temporary_consuming(Modifier& modifier, selection_id_t selection_id,
                                        move_delta_t delta) -> void {
    if (delta == move_delta_t {0, 0}) {
        return;
    }

    while (has_logicitem(modifier, selection_id)) {
        auto logicitem_id = get_first_logicitem(modifier, selection_id);
        modifier.remove_from_selection(selection_id, logicitem_id);

        modifier.move_or_delete_temporary_logicitem(logicitem_id, delta);
    }

    while (has_decoration(modifier, selection_id)) {
        auto decoration_id = get_first_decoration(modifier, selection_id);
        modifier.remove_from_selection(selection_id, decoration_id);

        modifier.move_or_delete_temporary_decoration(decoration_id, delta);
    }

    while (has_segment(modifier, selection_id)) {
        auto segment_part = get_first_segment(modifier, selection_id);
        modifier.remove_from_selection(selection_id, segment_part);

        modifier.move_or_delete_temporary_wire(segment_part, delta);
    }
}

auto delete_all(Modifier& modifier, selection_id_t selection_id) -> void {
    while (has_logicitem(modifier, selection_id)) {
        auto logicitem_id = get_first_logicitem(modifier, selection_id);
        modifier.remove_from_selection(selection_id, logicitem_id);

        modifier.change_logicitem_insertion_mode(logicitem_id, InsertionMode::temporary);
        modifier.delete_temporary_logicitem(logicitem_id);
    }

    while (has_decoration(modifier, selection_id)) {
        auto decoration_id = get_first_decoration(modifier, selection_id);
        modifier.remove_from_selection(selection_id, decoration_id);

        modifier.change_decoration_insertion_mode(decoration_id,
                                                  InsertionMode::temporary);
        modifier.delete_temporary_decoration(decoration_id);
    }

    while (has_segment(modifier, selection_id)) {
        auto segment_part = get_first_segment(modifier, selection_id);
        modifier.remove_from_selection(selection_id, segment_part);

        modifier.change_wire_insertion_mode(segment_part, InsertionMode::temporary);
        modifier.delete_temporary_wire_segment(segment_part);
    }
}

//
// History
//

auto is_history_enabled(const Modifier& modifier) -> bool {
    return editing::is_history_enabled(modifier.circuit_data().history);
}

auto has_undo(const Modifier& modifier) -> bool {
    return editing::has_undo_entries(modifier.circuit_data().history);
}

auto has_redo(const Modifier& modifier) -> bool {
    return editing::has_redo_entries(modifier.circuit_data().history);
}

auto has_ungrouped_undo_entries(const Modifier& modifier) -> bool {
    return editing::has_ungrouped_undo_entries(modifier.circuit_data().history);
}

auto undo_groups_count(const Modifier& modifier) -> std::size_t {
    return modifier.circuit_data().history.undo_stack.group_count();
}

}  // namespace editable_circuit

}  // namespace logicsim
