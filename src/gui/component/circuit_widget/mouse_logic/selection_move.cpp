#include "gui/component/circuit_widget/mouse_logic/selection_move.h"

#include "core/algorithm/round.h"
#include "core/editable_circuit.h"
#include "core/geometry/display_state_map.h"
#include "core/geometry/point.h"
#include "core/timer.h"

namespace logicsim {

namespace circuit_widget {

namespace {

auto add_to_selection(Selection& selection, const Layout& layout,
                      std::span<const SpatialIndex::value_t> items,
                      bool whole_tree) -> void {
    for (const auto& item : items) {
        if (item.is_logicitem()) {
            selection.add_logicitem(item.logicitem());
        } else if (item.is_segment()) {
            if (whole_tree) {
                add_segment_tree(selection, item.segment().wire_id, layout);
            } else {
                add_segment(selection, item.segment(), layout);
            }
        } else if (item.is_decoration()) {
            selection.add_decoration(item.decoration());
        }
    }
}

}  // namespace

SelectionMoveLogic::SelectionMoveLogic(const EditableCircuit& editable_circuit, Args args)
    : delete_on_cancel_ {args.delete_on_cancel},
      state_ {args.has_colliding ? State::waiting_for_confirmation
                                 : State::waiting_for_first_click},
      insertion_mode_ {args.has_colliding ? InsertionMode::collisions
                                          : InsertionMode::insert_or_discard},
      cross_points_ {std::move(args.cross_points)} {
    Expects(args.has_colliding == args.cross_points.has_value());

    // pre-conditions
    Expects(found_states_matches_insertion_mode(
        display_states(editable_circuit.visible_selection(), editable_circuit.layout()),
        insertion_mode_));
}

auto SelectionMoveLogic::mouse_press(EditableCircuit& editable_circuit,
                                     point_fine_t point, bool double_click) -> void {
    if (state_ == State::waiting_for_first_click) {
        const auto items = editable_circuit.query_selection(rect_fine_t {point, point});

        if (items.empty()) {
            editable_circuit.clear_visible_selection();
            state_ = State::finished;
            return;
        }

        if (!anything_selected(items, point, editable_circuit.visible_selection(),
                               editable_circuit.layout())) {
            auto selection = Selection {};
            add_to_selection(selection, editable_circuit.layout(), items, false);
            editable_circuit.set_visible_selection(selection);
        }

        if (double_click) {
            auto selection = Selection {editable_circuit.visible_selection()};
            add_to_selection(selection, editable_circuit.layout(), items, true);
            editable_circuit.set_visible_selection(selection);
        }
    }

    if (state_ == State::waiting_for_first_click ||
        state_ == State::waiting_for_confirmation) {
        state_ = State::move_selection;
        last_position_ = point;
    }
}

auto SelectionMoveLogic::mouse_move(EditableCircuit& editable_circuit,
                                    point_fine_t point) -> void {
    if (state_ != State::move_selection) {
        return;
    }

    move_selection(editable_circuit, point);
}

auto SelectionMoveLogic::mouse_release(EditableCircuit& editable_circuit,
                                       point_fine_t point) -> void {
    if (state_ != State::move_selection) {
        return;
    }
    const auto t [[maybe_unused]] = Timer {
        insertion_mode_ != InsertionMode::collisions ? "insert moved selection" : ""};

    move_selection(editable_circuit, point);
    convert_selection_to(editable_circuit, InsertionMode::collisions);

    if (anything_colliding(editable_circuit.visible_selection(),
                           editable_circuit.layout())) {
        state_ = State::waiting_for_confirmation;
    } else {
        state_ = State::finished;
    }
}

auto SelectionMoveLogic::is_finished() const -> bool {
    return state_ == State::finished || state_ == State::finished_confirmed;
}

auto SelectionMoveLogic::confirm() -> void {
    if (state_ != State::waiting_for_confirmation) {
        return;
    }

    state_ = State::finished_confirmed;
}

auto SelectionMoveLogic::finalize(EditableCircuit& editable_circuit) -> void {
    if (!is_finished()) {
        if (delete_on_cancel_) {
            editable_circuit.delete_all(editable_circuit.visible_selection());
        } else {
            restore_original_positions(editable_circuit);
        }
    }
    convert_selection_to(editable_circuit, InsertionMode::insert_or_discard);

    if (state_ == State::finished_confirmed) {
        editable_circuit.clear_visible_selection();
    }
}

auto SelectionMoveLogic::move_selection(EditableCircuit& editable_circuit,
                                        point_fine_t point) -> void {
    if (!last_position_) {
        return;
    }

    const auto delta_x = round_to<int>(double {point.x - last_position_->x});
    const auto delta_y = round_to<int>(double {point.y - last_position_->y});

    if (delta_x == 0 && delta_y == 0) {
        return;
    }
    const auto t [[maybe_unused]] =
        Timer {insertion_mode_ != InsertionMode::temporary ? "uninsert selection" : ""};

    if (!new_positions_representable(
            editable_circuit, editable_circuit.visible_selection(), delta_x, delta_y)) {
        return;
    }

    convert_selection_to(editable_circuit, InsertionMode::temporary);
    editable_circuit.move_temporary_unchecked(editable_circuit.visible_selection(),
                                              delta_x, delta_y);
    if (cross_points_) {
        cross_points_ = move_or_delete_points(cross_points_.value(), delta_x, delta_y);
    }

    *last_position_ += point_fine_t {delta_x, delta_y};
    total_offsets_.first += delta_x;
    total_offsets_.second += delta_y;
}

auto SelectionMoveLogic::convert_selection_to(EditableCircuit& editable_circuit,
                                              InsertionMode new_mode) -> void {
    Expects(found_states_matches_insertion_mode(
        display_states(editable_circuit.visible_selection(), editable_circuit.layout()),
        insertion_mode_));

    if (insertion_mode_ == new_mode) {
        return;
    }
    if (insertion_mode_ == InsertionMode::insert_or_discard && !cross_points_) {
        cross_points_.emplace(get_inserted_cross_points(
            editable_circuit, editable_circuit.visible_selection()));
    }
    if (insertion_mode_ == InsertionMode::temporary) {
        editable_circuit.split_temporary_before_insert(
            editable_circuit.visible_selection());
    }

    insertion_mode_ = new_mode;
    editable_circuit.apply_all_visible_selection_operations();
    editable_circuit.change_insertion_mode(editable_circuit.visible_selection(),
                                           new_mode);

    if (new_mode == InsertionMode::temporary) {
        editable_circuit.regularize_temporary_selection(
            editable_circuit.visible_selection(), cross_points_);
    }

    Ensures(found_states_matches_insertion_mode(
        display_states(editable_circuit.visible_selection(), editable_circuit.layout()),
        insertion_mode_));
}

auto SelectionMoveLogic::restore_original_positions(EditableCircuit& editable_circuit)
    -> void {
    if (total_offsets_.first == 0 && total_offsets_.second == 0) {
        return;
    }

    convert_selection_to(editable_circuit, InsertionMode::temporary);
    editable_circuit.move_temporary_unchecked(editable_circuit.visible_selection(),
                                              -total_offsets_.first,
                                              -total_offsets_.second);
}

}  // namespace circuit_widget

}  // namespace logicsim
