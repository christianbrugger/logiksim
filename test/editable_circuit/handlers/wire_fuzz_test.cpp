#include "./test_helpers.h"
#include "editable_circuit/handler_examples.h"
#include "editable_circuit/handlers.h"
#include "format.h"
#include "line_tree.h"
#include "timer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <range/v3/view/zip.hpp>

namespace logicsim {

//
// Add lines simple
//

auto test_add_many_wires(Rng& rng, bool random_modes) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    editable_circuit::examples::add_many_wires(rng, setup.state, random_modes);

    setup.validate();
}

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
// Add Lines Valid
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

auto get_insertion_result(Rng& rng, std::span<const ordered_line_t> lines) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

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

// TODO remove all enum formatters, and make them a template specialization
// TODO swap fmt_join arguments
// TODO rename new_valid, new_colliding, new_temporary, remove new_

namespace {
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
}  // namespace

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

    const auto lines = get_random_lines(rng, tries, 5, 10);
    const auto insertion_results = get_insertion_result(rng, lines);

    auto data = std::vector<TestLineData> {};

    for (auto [line, result] : ranges::zip_view(lines, insertion_results)) {
        if (result == InsertionResult::colliding) {
            if (get_random_bool(rng)) {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::temporary,
                    .expected_state = display_state_t::new_temporary,
                });
            } else {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::collisions,
                    .expected_state = display_state_t::new_colliding,
                });
            }
        } else {
            if (get_random_bool(rng)) {
                data.push_back(TestLineData {
                    .line = line,
                    .result = result,
                    .new_insertion_mode = InsertionMode::collisions,
                    .expected_state = display_state_t::new_valid,
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

    for (const auto element_id : layout.element_ids()) {
        const auto element_state = layout.display_state(element_id);
        const auto& tree = layout.segment_tree(element_id);

        if (is_inserted(element_state) && state == display_state_t::new_valid) {
            for (const auto index : tree.indices()) {
                std::ranges::copy(all_valid_lines(tree, index),
                                  std::back_inserter(result));
            }
        }

        else if (is_inserted(element_state) && state == display_state_t::normal) {
            std::ranges::copy(calculate_normal_lines(tree), std::back_inserter(result));
        }

        else if (element_state == state && !is_inserted(state)) {
            std::ranges::copy(all_lines(tree), std::back_inserter(result));
        }
    }
    return result;
}

auto test_valid_stability(Rng& rng) {
    auto circuit = empty_circuit();
    auto setup = HandlerSetup {circuit};

    const auto data = generate_insertable_line_data(rng);

    // insert data with new modes
    for (const auto entry : data) {
        const auto segment_part = editable_circuit::add_wire_segment(
            setup.state, entry.line, entry.new_insertion_mode);
    }
    setup.validate();

    // compare result
    for (const auto state : {
             display_state_t::new_temporary,
             display_state_t::new_colliding,
             display_state_t::new_valid,
             display_state_t::normal,
         }) {
        const auto expected_lines = get_expected_lines(data, state);
        const auto result_lines = get_all_lines(circuit.layout(), state);

        auto merged_expected = merge_lines(expected_lines);
        auto merged_result = merge_lines(result_lines);

        std::ranges::sort(merged_expected);
        std::ranges::sort(merged_result);

        if (merged_expected != merged_result) [[unlikely]] {
            print(state);
            print();
            print(fmt_join("{}", merged_expected, "\n"));
            print();
            print(fmt_join("{}", merged_result, "\n"));
            print();

            throw std::runtime_error("expected different lines with this state");
        }
    }
}

TEST(HandlerWireFuzz, AddAsValid) {
    for (auto i : range(50u)) {
        auto rng = Rng {i};

        test_valid_stability(rng);
    }
}

}  // namespace logicsim