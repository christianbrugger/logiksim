#include "main_editable_circuit.h"

#include "component/editable_circuit/handler.h"
#include "editable_circuit.h"
#include "format/std_type.h"
#include "geometry/point.h"
#include "layout_message.h"
#include "logging.h"
#include "random/wire.h"

#include <exception>

namespace logicsim {

//
// Editable Circuit
//

OldEditableCircuit::OldEditableCircuit() : OldEditableCircuit {Layout {}} {}

OldEditableCircuit::OldEditableCircuit(Layout&& layout)
    : layout_ {std::move(layout)},
      layout_index_ {layout_},
      sender_ {[](const auto&) {}},

      selection_store_ {},
      selection_builder_ {} {}

auto OldEditableCircuit::format() const -> std::string {
    return fmt::format("OldEditableCircuit{{\n{}}}", layout_);
}

auto OldEditableCircuit::layout() const -> const Layout& {
    return layout_;
}

auto OldEditableCircuit::extract_layout() -> Layout {
    auto temp = Layout {std::move(layout_)};
    *this = OldEditableCircuit {};
    return temp;
}

auto OldEditableCircuit::validate() -> void {
    if (LayoutIndex {layout_} != layout_index_) [[unlikely]] {
        throw std::runtime_error("layout index is out of sync");
    }

    selection_builder_.validate(layout_, layout_index_);

    for (const auto& item : selection_store_) {
        item.second.validate(layout_);
    }
}

auto OldEditableCircuit::add_example() -> void {
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

auto OldEditableCircuit::add_logic_item(const LogicItemDefinition& definition,
                                        point_t position, InsertionMode insertion_mode,
                                        selection_id_t selection_id) -> void {
    const auto logicitem_id = editable_circuit::add_logic_item(get_state(), definition,
                                                               position, insertion_mode);

    if (logicitem_id && selection_id) {
        selection(selection_id).add_logicitem(logicitem_id);
    }
}

auto OldEditableCircuit::add_line_segment(line_t line, InsertionMode insertion_mode,
                                          selection_id_t selection_id) -> void {
    auto* selection_ptr = selection_id ? &selection(selection_id) : nullptr;
    add_wire_segment(get_state(), selection_ptr, line, insertion_mode);
}

auto OldEditableCircuit::add_line_segments(point_t p0, point_t p1,
                                           LineInsertionType segment_type,
                                           InsertionMode insertion_mode,
                                           selection_id_t selection_id) -> void {
    auto* selection_ptr = selection_id ? &selection(selection_id) : nullptr;
    editable_circuit::add_wire(get_state(), p0, p1, segment_type, insertion_mode,
                               selection_ptr);
}

namespace {}  // namespace

auto OldEditableCircuit::new_positions_representable(const Selection& selection,
                                                     int delta_x, int delta_y) const
    -> bool {
    return editable_circuit::new_positions_representable(selection, layout_, delta_x,
                                                         delta_y);
}

auto OldEditableCircuit::move_or_delete(selection_id_t selection_id, int delta_x,
                                        int delta_y) -> void {
    move_or_delete(selection(selection_id), delta_x, delta_y);
}

auto OldEditableCircuit::move_or_delete(Selection selection__, int delta_x, int delta_y)
    -> void {
    const auto tracked_selection = ScopedSelection {*this, std::move(selection__)};
    auto& temp_selection = this->selection(tracked_selection.selection_id());

    editable_circuit::move_or_delete_elements(temp_selection, layout_, get_sender(),
                                              delta_x, delta_y);
}

auto OldEditableCircuit::change_insertion_mode(Selection selection__,
                                               InsertionMode new_insertion_mode) -> void {
    const auto tracked_selection = ScopedSelection {*this, std::move(selection__)};
    auto& temp_selection = this->selection(tracked_selection.selection_id());

    editable_circuit::change_insertion_mode(temp_selection, get_state(),
                                            new_insertion_mode);
}

auto OldEditableCircuit::change_insertion_mode(selection_id_t selection_id,
                                               InsertionMode new_insertion_mode) -> void {
    change_insertion_mode(selection(selection_id), new_insertion_mode);
}

auto OldEditableCircuit::move_unchecked(const Selection& selection, int delta_x,
                                        int delta_y) -> void {
    editable_circuit::move_unchecked(selection, layout_, delta_x, delta_y);
}

auto OldEditableCircuit::delete_all(selection_id_t selection_id) -> void {
    editable_circuit::delete_all(selection(selection_id), get_state());
}

auto OldEditableCircuit::delete_all(Selection selection__) -> void {
    const auto tracked_selection = ScopedSelection {*this, std::move(selection__)};
    auto& temp_selection = this->selection(tracked_selection.selection_id());

    editable_circuit::delete_all(temp_selection, get_state());
}

auto OldEditableCircuit::toggle_inverter(point_t point) -> void {
    editable_circuit::toggle_inverter(layout_, layout_index_, point);
}

auto OldEditableCircuit::toggle_wire_crosspoint(point_t point) -> void {
    editable_circuit::toggle_inserted_wire_crosspoint(get_state(), point);
}

auto OldEditableCircuit::set_attributes(logicitem_id_t logicitem_id,
                                        attributes_clock_generator_t attrs) -> void {
    layout_.logic_items().set_attributes(logicitem_id, std::move(attrs));
}

auto OldEditableCircuit::regularize_temporary_selection(
    const Selection& selection, std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t> {
    return editable_circuit::regularize_temporary_selection(layout_, get_sender(),
                                                            selection, true_cross_points);
}

auto OldEditableCircuit::capture_inserted_cross_points(const Selection& selection) const
    -> std::vector<point_t> {
    return editable_circuit::capture_inserted_cross_points(layout_, layout_index_,
                                                           selection);
}

auto OldEditableCircuit::split_before_insert(selection_id_t selection_id) -> void {
    split_before_insert(selection(selection_id));
}

auto OldEditableCircuit::split_before_insert(const Selection& selection) -> void {
    const auto split_points =
        editable_circuit::capture_new_splitpoints(layout_, layout_index_, selection);

    editable_circuit::split_temporary_segments(layout_, get_sender(),
                                               std::move(split_points), selection);
}

auto OldEditableCircuit::selection_count() const -> std::size_t {
    return selection_store_.size();
}

auto OldEditableCircuit::selection(selection_id_t selection_id) -> Selection& {
    return selection_store_.at(selection_id);
}

auto OldEditableCircuit::selection(selection_id_t selection_id) const
    -> const Selection& {
    return selection_store_.at(selection_id);
}

auto OldEditableCircuit::create_selection() -> selection_id_t {
    return selection_store_.create();
}

auto OldEditableCircuit::create_selection(Selection selection__) -> selection_id_t {
    const auto selection_id = create_selection();
    this->selection(selection_id) = std::move(selection__);
    return selection_id;
}

auto OldEditableCircuit::destroy_selection(selection_id_t selection_id) -> void {
    selection_store_.destroy(selection_id);
}

auto OldEditableCircuit::selection_exists(selection_id_t selection_id) const -> bool {
    return selection_store_.contains(selection_id);
}

auto OldEditableCircuit::set_visible_selection(Selection selection) -> void {
    selection_builder_.set_selection(std::move(selection));
}

auto OldEditableCircuit::clear_visible_selection() -> void {
    selection_builder_.clear();
}

auto OldEditableCircuit::add_visible_selection_rect(SelectionFunction function,
                                                    rect_fine_t rect) -> void {
    selection_builder_.add(function, rect);
}

auto OldEditableCircuit::try_pop_last_visible_selection_rect() -> bool {
    if (selection_builder_.operation_count() == size_t {0}) {
        return false;
    }
    selection_builder_.pop_last();
    return true;
}

auto OldEditableCircuit::try_update_last_visible_selection_rect(rect_fine_t rect)
    -> bool {
    if (selection_builder_.operation_count() == size_t {0}) {
        return false;
    }
    selection_builder_.update_last(rect);
    return true;
}

auto OldEditableCircuit::apply_all_visible_selection_operations() -> void {
    selection_builder_.apply_all_operations(layout_, layout_index_);
}

auto OldEditableCircuit::visible_selection() const -> const Selection& {
    return selection_builder_.selection(layout_, layout_index_);
}

auto OldEditableCircuit::visible_selection_empty() const -> bool {
    return selection_builder_.empty();
}

auto OldEditableCircuit::caches() const -> const LayoutIndex& {
    return layout_index_;
}

auto OldEditableCircuit::submit(const InfoMessage& message) -> void {
    layout_index_.submit(message);
    selection_builder_.submit(message);

    for (auto& item : selection_store_) {
        item.second.submit(message);
    }
}

auto OldEditableCircuit::get_sender() -> editable_circuit::MessageSender& {
    // lambda contains the wrong 'this' pointer after copy or move
    sender_ = editable_circuit::MessageSender {
        [this](const InfoMessage& message) { this->submit(message); }};
    return sender_;
}

auto OldEditableCircuit::get_state() -> editable_circuit::State {
    return editable_circuit::State {layout_, get_sender(), layout_index_};
}

//
// Scoped Selection
//

ScopedSelection::ScopedSelection(OldEditableCircuit& editable_circuit)
    : editable_circuit_ {&editable_circuit},
      selection_id_ {editable_circuit.create_selection()} {
    Ensures(selection_id_);
}

ScopedSelection::ScopedSelection(OldEditableCircuit& editable_circuit,
                                 Selection selection__)
    : editable_circuit_ {&editable_circuit},
      selection_id_ {editable_circuit.create_selection(std::move(selection__))} {
    Ensures(selection_id_);
}

ScopedSelection::~ScopedSelection() {
    Expects(selection_id_);
    editable_circuit_->destroy_selection(selection_id_);
}

auto ScopedSelection::selection_id() const -> selection_id_t {
    Expects(selection_id_);
    return selection_id_;
}

//
// Free functions
//

auto save_delete_all(OldEditableCircuit& editable_circuit, selection_id_t selection_id)
    -> void {
    if (editable_circuit.selection_exists(selection_id)) {
        editable_circuit.delete_all(selection_id);
    }
}

auto save_destroy_selection(OldEditableCircuit& editable_circuit,
                            selection_id_t selection_id) -> void {
    if (editable_circuit.selection_exists(selection_id)) {
        editable_circuit.destroy_selection(selection_id);
    }
}

auto visible_selection_select_all(OldEditableCircuit& editable_circuit) -> void {
    const auto rect = rect_fine_t {point_fine_t {grid_t::min(), grid_t::min()},
                                   point_fine_t {grid_t::max(), grid_t::max()}};

    editable_circuit.clear_visible_selection();
    editable_circuit.add_visible_selection_rect(SelectionFunction::add, rect);
}

auto visible_selection_delete_all(OldEditableCircuit& editable_circuit) -> void {
    // Clear the visible selection before deleting for optimization.
    // So it is not tracked during deletion. (10% speedup)

    auto selection__ = Selection {editable_circuit.visible_selection()};
    editable_circuit.clear_visible_selection();
    editable_circuit.delete_all(std::move(selection__));
}

}  // namespace logicsim
