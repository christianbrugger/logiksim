#include "./test_helpers.h"
#include "algorithm/uniform_int_distribution.h"
#include "component/editable_circuit/handler.h"
#include "editable_circuit.h"
#include "exception.h"
#include "geometry/line.h"
#include "line_tree.h"
#include "random/bool.h"
#include "random/ordered_line.h"
#include "random/segment.h"
#include "random/wire.h"
#include "selection_normalization.h"
#include "timer.h"
#include "tree_normalization.h"
#include "vocabulary/segment_part.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <range/v3/view/zip.hpp>

namespace logicsim {

//
// Add lines simple
//

namespace {
auto test_add_many_wires(Rng& rng, bool random_modes) {
    auto editable_circuit = EditableCircuit {};
    add_many_wires(rng, editable_circuit, random_modes);
}
}  // namespace

TEST(HandlerWireFuzz, AddTempSegmentRandomModes) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_add_many_wires(rng, true);
    }
}

TEST(HandlerWireFuzz, AddTempSegmentInsertionModes) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_add_many_wires(rng, false);
    }
}

//
// Add Lines and Check State
//

enum class InsertionResult {
    colliding,
    valid,
};

template <>
auto format(InsertionResult value) -> std::string {
    if (value == InsertionResult::colliding) {
        return "colliding";
    }
    if (value == InsertionResult::valid) {
        return "valid";
    }
    throw_exception("unknown InsertionResult value");
}

namespace {
auto get_insertion_result(std::span<const ordered_line_t> lines) {
    auto layout = Layout {};
    auto setup = HandlerSetup {layout};

    auto result = std::vector<InsertionResult> {};

    for (const auto line : lines) {
        const auto segment_part = editable_circuit::add_wire_segment(
            setup.state, line, InsertionMode::insert_or_discard);

        result.push_back(segment_part ? InsertionResult::valid
                                      : InsertionResult::colliding);
    }

    setup.validate();
    return result;
}

struct TestLineData {
    ordered_line_t line;
    InsertionResult result;
    InsertionMode new_insertion_mode;
    display_state_t expected_state;

    auto format() const -> std::string {
        return fmt::format("({}, {}, {}, {})", line, result, new_insertion_mode,
                           expected_state);
    }
};

auto get_expected_lines(std::span<const TestLineData> data, display_state_t state) {
    auto result = std::vector<ordered_line_t> {};

    for (const auto& entry : data) {
        if (entry.expected_state == state) {
            result.push_back(entry.line);
        }
    }

    return result;
}

auto generate_insertable_line_data(Rng& rng) {
    const auto tries = uint_distribution(5, 100)(rng);

    const auto lines = get_random_ordered_lines(rng, tries, grid_t {5}, grid_t {10});
    const auto insertion_results = get_insertion_result(lines);

    auto data = std::vector<TestLineData> {};

    for (auto [line, result] : ranges::zip_view(lines, insertion_results)) {
        if (result == InsertionResult::colliding) {
            if (get_random_bool(rng)) {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::temporary,
                    .expected_state = display_state_t::temporary,
                });
            } else {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::collisions,
                    .expected_state = display_state_t::colliding,
                });
            }
        } else {
            if (get_random_bool(rng)) {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::collisions,
                    .expected_state = display_state_t::valid,
                });
            } else {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::insert_or_discard,
                    .expected_state = display_state_t::normal,
                });
            }
        }
    }

    return data;
}

auto get_all_lines(const Layout& layout, display_state_t state)
    -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto wire_id : wire_ids(layout)) {
        const auto& tree = layout.wires().segment_tree(wire_id);

        if (is_inserted(wire_id) && state == display_state_t::valid) {
            for (const auto index : tree.indices()) {
                std::ranges::copy(all_valid_lines(tree, index),
                                  std::back_inserter(result));
            }
        }

        else if (is_inserted(wire_id) && state == display_state_t::normal) {
            std::ranges::copy(calculate_normal_lines(tree), std::back_inserter(result));
        }

        else if (to_display_state(wire_id) == state && !is_inserted(wire_id)) {
            std::ranges::copy(all_lines(tree), std::back_inserter(result));
        }
    }
    return result;
}

auto test_add_wire_states_correct(Rng& rng) {
    auto layout = Layout {};
    auto setup = HandlerSetup {layout};

    auto data = generate_insertable_line_data(rng);

    // insert data with new modes
    for (const auto entry : data) {
        const auto segment_part = editable_circuit::add_wire_segment(
            setup.state, entry.line, entry.new_insertion_mode);

        if (!segment_part) [[unlikely]] {
            throw_exception("wasn't able to insert line that should be insertable");
        }
        if (distance(segment_part.part) != distance(entry.line)) [[unlikely]] {
            throw_exception("returned segment has different size than given line");
        }
        if (get_line(layout, segment_part) != entry.line) [[unlikely]] {
            throw_exception("the line the segment points to is different");
        }
    }
    setup.validate();

    // compare result
    for (const auto state : {
             display_state_t::temporary,
             display_state_t::colliding,
             display_state_t::valid,
             display_state_t::normal,
         }) {
        const auto expected_lines = merge_split_segments(get_expected_lines(data, state));
        const auto result_lines = merge_split_segments(get_all_lines(layout, state));

        if (expected_lines != result_lines) [[unlikely]] {
            throw std::runtime_error("expected different lines with this state");
        }
    }
}
}  // namespace

TEST(HandlerWireFuzz, AddWireStatesCorrect) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_add_wire_states_correct(rng);
    }
}

//
// Remove lines
//

namespace {
auto test_remove_many_wires(Rng& rng, bool random_modes) {
    auto editable_circuit = EditableCircuit {};
    add_many_wires(rng, editable_circuit, random_modes);

    while (true) {
        const auto segment = get_random_segment(rng, editable_circuit.layout());
        if (!segment) {
            break;
        }
        const auto part = to_part(get_line(editable_circuit.layout(), segment));
        auto segment_part = segment_part_t {segment, part};

        const auto guard = SelectionGuard {editable_circuit};
        editable_circuit.add_to_selection(guard.selection_id(), segment_part);
        editable_circuit.change_insertion_mode(guard.selection_id(),
                                               InsertionMode::temporary);
        {
            const auto& selection = editable_circuit.selection(guard.selection_id());
            if (selection.selected_segments().size() != 1 ||
                selection.selected_segments().front().second.size() != 1 ||
                selection.selected_segments().front().second.front() != part) {
                throw_exception("unexpected segment state");
            }
        }

        editable_circuit.delete_all(guard.selection_id());
        if (!editable_circuit.selection(guard.selection_id()).empty()) {
            throw_exception("selection should be empty");
        }
    }

    if (has_segments(editable_circuit.layout())) {
        throw_exception("layout should be empty at this point");
    }
}
}  // namespace

TEST(HandlerWireFuzz, RemoveManyInsertedWires) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_remove_many_wires(rng, false);
    }
}

TEST(HandlerWireFuzz, RemoveManyWiresDifferentModes) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_remove_many_wires(rng, true);
    }
}

//
// Remove wires partially
//

namespace {
auto test_remove_partial_wires(Rng& rng, bool random_modes) {
    auto editable_circuit = EditableCircuit {};
    add_many_wires(rng, editable_circuit, random_modes);

    while (true) {
        auto segment_part = get_random_segment_part(rng, editable_circuit.layout());
        if (!segment_part) {
            break;
        }
        segment_part = [&]() {
            const auto& circuit = editable_circuit.modifier().circuit_data();
            return sanitize_part(segment_part, circuit.layout,
                                 circuit.index.collision_index(), SanitizeMode::expand);
        }();

        if (!segment_part) {
            throw_exception("invalid segment part");
        }
        const auto orig_distance = distance(segment_part.part);

        const auto guard = SelectionGuard {editable_circuit};
        editable_circuit.add_to_selection(guard.selection_id(), segment_part);
        editable_circuit.change_insertion_mode(guard.selection_id(),
                                               InsertionMode::temporary);

        {
            const auto& selection = editable_circuit.selection(guard.selection_id());
            if (selection.selected_segments().size() != 1 ||
                selection.selected_segments().front().second.size() != 1 ||
                distance(selection.selected_segments().front().second.front()) !=
                    orig_distance) {
                throw_exception("unexpected segment state");
            }
        }

        editable_circuit.delete_all(guard.selection_id());
        if (!editable_circuit.selection(guard.selection_id()).empty()) {
            throw_exception("selection should be empty");
        }
    }

    if (has_segments(editable_circuit.layout())) {
        throw_exception("layout should be empty at this point");
    }
}
}  // namespace

TEST(HandlerWireFuzz, RemovePartialInsertedWires) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_remove_partial_wires(rng, false);
    }
}

//
// Add lines and buttons
//

namespace {
auto test_add_wires_buttons(Rng& rng, bool random_modes) {
    auto editable_circuit = EditableCircuit {};

    add_many_wires_and_buttons(rng, editable_circuit,
                               WiresButtonsParams {.random_modes = random_modes});
}
}  // namespace

TEST(HandlerWireFuzz, AddWiresAndButtonsRandomModes) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_add_wires_buttons(rng, true);
    }
}

TEST(HandlerWireFuzz, AddWiresAndButtonsNormal) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};
        test_add_wires_buttons(rng, false);
    }
}

}  // namespace logicsim