
#include "core/algorithm/vector_operations.h"
#include "core/component/editable_circuit/key_state.h"
#include "core/component/editable_circuit/modifier.h"
#include "core/concept/integral.h"
#include "core/logging.h"
#include "core/random/fuzz.h"

#include <cstdint>
#include <span>

namespace logicsim {

namespace editable_circuit {

namespace {

auto add_wire_segment(FuzzStream& stream, Modifier& modifier) -> void {
    const bool horizontal = fuzz_bool(stream);

    const auto a = fuzz_small_int(stream, 0, 4);
    const auto b = fuzz_small_int(stream, 1, 5 - a);
    const auto c = fuzz_small_int(stream, 0, 5);

    const auto start = a;
    const auto end = a + b;
    const auto pos = c;

    const auto line = horizontal
                          ? ordered_line_t {point_t {start, pos}, point_t {end, pos}}
                          : ordered_line_t {point_t {pos, start}, point_t {pos, end}};

    const auto mode = [&stream] {
        switch (fuzz_small_int(stream, 0, 2)) {
            case 0:
                return InsertionMode::insert_or_discard;
            case 1:
                return InsertionMode::collisions;
            case 2:
                return InsertionMode::temporary;
        }
        std::terminate();
    }();

    modifier.add_wire_segment(line, mode);
}

auto delete_temporary_wire_segment(FuzzStream& stream, Modifier& modifier) -> void {
    const auto temporary_count =
        get_temporary_segment_count(modifier.circuit_data().layout);

    if (temporary_count == 0) {
        return;
    }

    // pick random segment
    const auto max_segment_index = clamp_to_fuzz_stream(temporary_count - 1);
    const auto segment_index =
        segment_index_t {fuzz_small_int(stream, 0, max_segment_index)};
    const auto segment = segment_t {temporary_wire_id, segment_index};

    // pick random part
    const auto part_full = get_part(modifier.circuit_data().layout, segment);
    const auto max_offset = clamp_to_fuzz_stream(part_full.end.value);
    const auto a = fuzz_small_int(stream, 0, max_offset - 1);
    const auto b = fuzz_small_int(stream, a + 1, max_offset);
    const auto part = part_t {a, b};

    auto segment_part = segment_part_t {segment, part};
    modifier.delete_temporary_wire_segment(segment_part);
}

auto editing_operation(FuzzStream& stream, Modifier& modifier) -> void {
    switch (fuzz_small_int(stream, 0, 1)) {
        case 0:
            add_wire_segment(stream, modifier);
            return;
        case 1:
            delete_temporary_wire_segment(stream, modifier);
            return;
    }
    std::terminate();
}

auto validate_undo_redo(Modifier& modifier,
                        const std::vector<layout_key_state_t>& key_state_stack) {
    Expects(key_state_stack.empty() ||
            layout_key_state_t {modifier} == key_state_stack.back());
    Expects(!modifier.has_ungrouped_undo_entries());

    // undo completely
    for (const auto& state : std::ranges::views::reverse(key_state_stack)  //
                                 | std::ranges::views::drop(1)) {
        Expects(modifier.has_undo());
        modifier.undo_group();
        Expects(layout_key_state_t {modifier} == state);
    }
    Expects(!modifier.has_undo());

    // redo completely
    for (const auto& state : key_state_stack | std::ranges::views::drop(1)) {
        Expects(modifier.has_redo());
        modifier.redo_group();
        Expects(layout_key_state_t {modifier} == state);
    }
    Expects(!modifier.has_redo());
}

auto process_data(std::span<const uint8_t> data) -> void {
    auto stream = FuzzStream(data);

    auto modifier = Modifier {Layout {}, ModifierConfig {
                                             .enable_history = true,
                                             .validate_messages = true,
                                         }};
    auto key_state_stack = std::vector<layout_key_state_t> {
        layout_key_state_t {modifier},
    };

    while (!stream.empty()) {
        editing_operation(stream, modifier);

        // store history state
        if (modifier.has_ungrouped_undo_entries()) {
            modifier.finish_undo_group();
            key_state_stack.emplace_back(modifier);
        }
    }

    validate_undo_redo(modifier, key_state_stack);
}

}  // namespace

}  // namespace editable_circuit

}  // namespace logicsim

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) -> int {
    using namespace logicsim::editable_circuit;
    const auto span = std::span<const uint8_t> {data, size};

    process_data(span);

    return 0;
}
