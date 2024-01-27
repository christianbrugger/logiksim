
#include "algorithm/range.h"
#include "algorithm/uniform_int_distribution.h"
#include "editable_circuit.h"
#include "format/container.h"
#include "geometry/display_state_map.h"
#include "logging.h"
#include "random/bool.h"
#include "random/generator.h"
#include "random/insertion_mode.h"
#include "random/point.h"
#include "render_circuit.h"
#include "timer.h"
#include "tree_normalization.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/core.h>

namespace logicsim {

namespace {

struct AddResult {
    point_t p0;
    point_t p1;
    LineInsertionType line_segment_type;
    InsertionMode insertion_mode;
    selection_id_t selection_id;
    std::vector<ordered_line_t> sorted_inserted_lines;

    auto format() const -> std::string {
        return fmt::format(
            "AddResult(\n"
            "    p0 = {}, p1 = {}\n"
            "    line_segment_type = {}\n"
            "    insertion_mode = {}\n"
            "    selection_id = {}\n"
            "    sorted_inserted_lines = {}\n"
            ")\n",
            p0, p1, line_segment_type, insertion_mode, selection_id,
            sorted_inserted_lines);
    }
};

auto get_sorted_lines(const Selection &selection, const Layout &layout) {
    auto lines = merge_split_segments(get_lines(selection, layout));
    std::ranges::sort(lines);
    return lines;
}

auto validate_inserted_lines(const AddResult &result) {
    // check count
    const auto count = result.sorted_inserted_lines.size();
    const auto expected_count = [&] {
        if (result.p0 == result.p1) {
            return std::size_t {0};
        }
        if (is_orthogonal_line(result.p0, result.p1)) {
            return std::size_t {1};
        }
        return std::size_t {2};
    }();

    if (result.insertion_mode == InsertionMode::insert_or_discard) {
        if (count > expected_count) {
            throw_exception("wrong line count");
        }
    } else {
        if (count != expected_count) {
            throw_exception("wrong line count");
        }
    }

    // check endpoints
    const auto p0 = result.p0;
    const auto p1 = result.p1;

    for (const auto line : result.sorted_inserted_lines) {
        if (line.p0 != p0 && line.p0 != p1 && line.p1 != p0 && line.p1 != p1) {
            throw_exception("line is not related to given points");
        }
    }
}

auto add_random_line(Rng &rng, EditableCircuit &editable_circuit, bool random_modes)
    -> AddResult {
    const auto min = grid_t {5};
    const auto max = grid_t {10};

    const auto p0 = get_random_point(rng, min, max);
    const auto p1 = get_random_point(rng, min, max);

    const auto type = get_random_bool(rng) ? LineInsertionType::horizontal_first
                                           : LineInsertionType::vertical_first;
    const auto mode =
        random_modes ? get_random_insertion_mode(rng) : InsertionMode::insert_or_discard;

    const auto selection_id = editable_circuit.create_selection();
    editable_circuit.add_line_segments(p0, p1, type, mode, selection_id);

    auto lines = get_sorted_lines(editable_circuit.selection(selection_id),
                                  editable_circuit.layout());
    std::ranges::sort(lines);
    auto result = AddResult {p0, p1, type, mode, selection_id, lines};

    validate_inserted_lines(result);

    // print("Inserted", result);
    return result;
}

auto verify_selection(const EditableCircuit &editable_circuit, const AddResult &result) {
    auto lines = get_sorted_lines(editable_circuit.selection(result.selection_id),
                                  editable_circuit.layout());

    // print(result);
    // print("ACTUAL =", lines);

    if (lines != result.sorted_inserted_lines) {
        throw_exception("lines are now different than when inserted");
    }
}

auto verify_selections(const EditableCircuit &editable_circuit,
                       const std::vector<AddResult> &data) {
    for (auto &result : data) {
        verify_selection(editable_circuit, result);
    }
}

auto add_many_wires(Rng &rng, EditableCircuit &editable_circuit, bool random_modes)
    -> std::vector<AddResult> {
    auto data = std::vector<AddResult> {};

    const auto tries = uint_distribution(5, 100)(rng);  // TODO re-enable

    for (auto _ [[maybe_unused]] : range(tries)) {
        auto result = add_random_line(rng, editable_circuit, random_modes);
        data.emplace_back(std::move(result));

        // print();
        // print(editable_circuit.circuit());
        // print();
        // verify_selections(data, editable_circuit.circuit().layout());
    }
    // print();
    // print();
    // print();
    // print(editable_circuit);
    // print();
    // print();
    // print();

    verify_selections(editable_circuit, data);

    return data;
}

auto test_add_many_wires(Rng &rng, bool random_modes) {
    auto editable_circuit = EditableCircuit {Layout {}};

    add_many_wires(rng, editable_circuit, random_modes);

    editable_circuit.validate();
}

}  // namespace

//  "args": [ "--gtest_filter=*AddRandomWiresInserted" ]

#include <iostream>

TEST(EditableCircuitRandom, AddRandomWiresInserted) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};
        test_add_many_wires(rng, false);
    }
}

TEST(EditableCircuitRandom, AddRandomWiresRandomMode) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};
        test_add_many_wires(rng, true);
    }
}

//
// Move back and forth
//

namespace {

auto copy_selection(EditableCircuit &editable_circuit, const Selection &selection)
    -> selection_id_t {
    const auto selection_id = editable_circuit.create_selection();
    editable_circuit.selection(selection_id) = selection;
    return selection_id;
}

}  // namespace

class TrackedSelection {
   public:
    TrackedSelection(EditableCircuit &editable_circuit, selection_id_t selection_id,
                     InsertionMode starting_mode,
                     std::optional<std::vector<point_t>> cross_points = {})
        : editable_circuit_(editable_circuit),
          selection_id_(std::move(selection_id)),
          insertion_mode_(starting_mode),
          cross_points_ {std::move(cross_points)} {}

    TrackedSelection(EditableCircuit &editable_circuit, const Selection &selection,
                     InsertionMode starting_mode,
                     std::optional<std::vector<point_t>> cross_points = {})
        : TrackedSelection(editable_circuit, copy_selection(editable_circuit, selection),
                           starting_mode, cross_points) {}

    auto convert_to(InsertionMode new_mode) -> void {
        Ensures(found_states_matches_insertion_mode(
            display_states(editable_circuit_.selection(selection_id_),
                           editable_circuit_.layout()),
            insertion_mode_));

        if (insertion_mode_ == new_mode) {
            return;
        }
        if (insertion_mode_ == InsertionMode::insert_or_discard && !cross_points_) {
            cross_points_.emplace(editable_circuit_.capture_inserted_cross_points(
                editable_circuit_.selection(selection_id_)));
        }
        if (insertion_mode_ == InsertionMode::temporary) {
            editable_circuit_.split_before_insert(
                editable_circuit_.selection(selection_id_));
        }

        Ensures(found_states_matches_insertion_mode(
            display_states(editable_circuit_.selection(selection_id_),
                           editable_circuit_.layout()),
            insertion_mode_));

        insertion_mode_ = new_mode;
        editable_circuit_.change_insertion_mode(selection_id_, new_mode);

        Ensures(found_states_matches_insertion_mode(
            display_states(editable_circuit_.selection(selection_id_),
                           editable_circuit_.layout()),
            insertion_mode_));

        if (new_mode == InsertionMode::temporary) {
            editable_circuit_.regularize_temporary_selection(
                editable_circuit_.selection(selection_id_), cross_points_);
        }

        Ensures(found_states_matches_insertion_mode(
            display_states(editable_circuit_.selection(selection_id_),
                           editable_circuit_.layout()),
            insertion_mode_));
    }

    auto move_or_delete(int delta_x, int delta_y) -> void {
        editable_circuit_.move_or_delete(selection_id_, delta_x, delta_y);
    }

    auto move_unchecked(int delta_x, int delta_y) -> void {
        editable_circuit_.move_unchecked(editable_circuit_.selection(selection_id_),
                                         delta_x, delta_y);
    }

   private:
    EditableCircuit &editable_circuit_;
    selection_id_t selection_id_;
    InsertionMode insertion_mode_;
    std::optional<std::vector<point_t>> cross_points_;
};

namespace {
auto test_move_wires_back_and_forth(unsigned int seed, Rng &rng, bool do_render = false) {
    auto editable_circuit = EditableCircuit {Layout {}};

    editable_circuit.add_example();
    editable_circuit.validate();

    auto expected_layout = moved_layout(editable_circuit.layout(), 10, 10).value();

    // First move
    editable_circuit.add_visible_selection_rect(
        SelectionFunction::add, rect_fine_t {point_fine_t {5, 5}, point_fine_t {7, 7}});
    auto tracker_1 =
        TrackedSelection {editable_circuit, editable_circuit.visible_selection(),
                          InsertionMode::insert_or_discard};
    tracker_1.convert_to(InsertionMode::temporary);
    tracker_1.move_unchecked(10, 10);
    tracker_1.convert_to(InsertionMode::insert_or_discard);
    editable_circuit.validate();

    // Mark rest as temporary
    editable_circuit.clear_visible_selection();
    editable_circuit.add_visible_selection_rect(
        SelectionFunction::add, rect_fine_t {point_fine_t {5, 5}, point_fine_t {10, 10}});
    auto tracker_2 =
        TrackedSelection {editable_circuit, editable_circuit.visible_selection(),
                          InsertionMode::insert_or_discard};
    tracker_2.convert_to(InsertionMode::temporary);
    editable_circuit.validate();

    // Add example and colliding
    editable_circuit.add_example();
    tracker_2.convert_to(InsertionMode::collisions);
    editable_circuit.validate();

    // Move second part
    tracker_2.convert_to(InsertionMode::temporary);
    tracker_2.move_unchecked(10, 10);
    tracker_2.convert_to(InsertionMode::insert_or_discard);
    editable_circuit.validate();

    // delete example
    editable_circuit.delete_all(editable_circuit.visible_selection());

    auto final_layout = Layout {editable_circuit.layout()};
    expected_layout.normalize();
    final_layout.normalize();

    if (final_layout != expected_layout) {
        print("final_layout:", final_layout);
        print("expected_layout:", expected_layout);

        throw_exception("final layouts are not the same");
    }

    if (do_render) {
        render_layout_to_file(editable_circuit.layout(), 400, 400,
                              fmt::format("test_move/{:04d}.png", seed));
    }
};
}  // namespace

TEST(EditableCircuitRandom, MoveWiresBackAndForth) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_move_wires_back_and_forth(i, rng);
    }
}

}  // namespace logicsim