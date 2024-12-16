
#include "core/algorithm/range.h"
#include "core/algorithm/span_operations.h"
#include "core/component/editable_circuit/key_state.h"
#include "core/component/editable_circuit/modifier.h"
#include "core/geometry/layout_geometry.h"
#include "core/geometry/rect.h"
#include "core/logging.h"
#include "core/random/fuzz.h"
#include "core/selection_sanitization.h"

#include <cstdint>
#include <span>

namespace logicsim {

namespace editable_circuit {

namespace {

struct FuzzLimits {
    rect_t box {
        point_t {0, 0},
        point_t {5, 5},
    };

    [[nodiscard]] auto operator==(const FuzzLimits&) const -> bool = default;
};

[[nodiscard]] auto all_within_limits(const Layout& layout, FuzzLimits limits) -> bool {
    return enclosing_rect(bounding_rect(layout), limits.box) == limits.box;
}

auto fuzz_select_insertion_mode(FuzzStream& stream) -> InsertionMode {
    switch (fuzz_small_int(stream, 0, 2)) {
        case 0:
            return InsertionMode::insert_or_discard;
        case 1:
            return InsertionMode::collisions;
        case 2:
            return InsertionMode::temporary;
    }
    std::terminate();
}

auto fuzz_select_insertion_hint(FuzzStream& stream,
                                InsertionMode new_mode) -> SegmentInsertionHint {
    if (new_mode == InsertionMode::temporary) {
        return SegmentInsertionHint::no_hint;
    }

    switch (fuzz_small_int(stream, 0, 1)) {
        case 0:
            return SegmentInsertionHint::no_hint;
        case 1:
            return SegmentInsertionHint::assume_colliding;
    }
    std::terminate();
}

auto fuzz_select_temporary_segment(FuzzStream& stream,
                                   Modifier& modifier) -> std::optional<segment_t> {
    const auto temporary_count =
        get_temporary_segment_count(modifier.circuit_data().layout);

    if (temporary_count == 0) {
        return std::nullopt;
    }

    const auto max_segment_index = clamp_to_fuzz_stream(temporary_count - 1);
    return segment_t {
        temporary_wire_id,
        segment_index_t {fuzz_small_int(stream, 0, max_segment_index)},
    };
}

auto fuzz_select_segment(FuzzStream& stream,
                         Modifier& modifier) -> std::optional<segment_t> {
    const auto& segments = modifier.circuit_data().index.key_index().segments();

    if (segments.empty()) {
        return std::nullopt;
    }

    const auto max_index = clamp_to_fuzz_stream(segments.size() - 1);
    const auto index = fuzz_small_int(stream, 0, max_index);
    return checked_at(segments, index).first;
}

auto fuzz_select_part(FuzzStream& stream, Modifier& modifier,
                      segment_t segment) -> part_t {
    const auto part_full = get_part(modifier.circuit_data().layout, segment);
    const auto max_offset = clamp_to_fuzz_stream(part_full.end.value);
    const auto a = fuzz_small_int(stream, 0, max_offset - 1);
    const auto b = fuzz_small_int(stream, a + 1, max_offset);
    return part_t {a, b};
}

auto fuzz_select_move_delta(FuzzStream& stream, ordered_line_t line,
                            const FuzzLimits& limits) -> move_delta_t {
    return move_delta_t {
        .x = fuzz_small_int(stream,  //
                            int {limits.box.p0.x} - int {line.p0.x},
                            int {limits.box.p1.x} - int {line.p1.x}),
        .y = fuzz_small_int(stream,  //
                            int {limits.box.p0.y} - int {line.p0.y},
                            int {limits.box.p1.y} - int {line.p1.y}),
    };
}

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

    const auto mode = fuzz_select_insertion_mode(stream);

    modifier.add_wire_segment(line, mode);
}

auto delete_temporary_wire_segment(FuzzStream& stream, Modifier& modifier) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto part = fuzz_select_part(stream, modifier, segment.value());

        auto segment_part = segment_part_t {segment.value(), part};
        modifier.delete_temporary_wire_segment(segment_part);
    }
}

auto change_wire_insertion_mode(FuzzStream& stream, Modifier& modifier) -> void {
    if (const auto segment = fuzz_select_segment(stream, modifier)) {
        const auto part = fuzz_select_part(stream, modifier, segment.value());
        const auto new_mode = fuzz_select_insertion_mode(stream);

        auto segment_part = segment_part_t {segment.value(), part};

        if (change_wire_insertion_mode_requires_sanitization(segment_part, new_mode)) {
            const auto sanitize_mode =
                fuzz_bool(stream) ? SanitizeMode::expand : SanitizeMode::shrink;
            segment_part = sanitize_part(segment_part, modifier, sanitize_mode);
        }

        if (segment_part) {
            const auto hint = fuzz_select_insertion_hint(stream, new_mode);
            modifier.change_wire_insertion_mode(segment_part, new_mode, hint);
        }
    }
}

auto move_temporary_wire_unchecked(FuzzStream& stream, Modifier& modifier,
                                   const FuzzLimits& limits) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto segment_part =
            get_segment_part(modifier.circuit_data().layout, *segment);

        const auto line = get_line(modifier.circuit_data().layout, *segment);
        const auto delta = fuzz_select_move_delta(stream, line, limits);

        modifier.move_temporary_wire_unchecked(segment_part, delta);
    }
}

auto move_or_delete_temporary_wire(FuzzStream& stream, Modifier& modifier,
                                   const FuzzLimits& limits) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto part = fuzz_select_part(stream, modifier, *segment);
        auto segment_part = segment_part_t {*segment, part};

        const auto line = get_line(modifier.circuit_data().layout, segment_part);
        const auto delta = fuzz_select_move_delta(stream, line, limits);

        modifier.move_or_delete_temporary_wire(segment_part, delta);
    }
}

auto set_visible_selection(FuzzStream& stream, Modifier& modifier) -> void {
    auto selection = Selection {};

    for (const auto _ : range(fuzz_small_int(stream, 0, 4))) {
        if (const auto segment = fuzz_select_segment(stream, modifier)) {
            const auto part = fuzz_select_part(stream, modifier, *segment);
            selection.add_segment(segment_part_t {*segment, part});
        }
    }

    modifier.set_visible_selection(std::move(selection));
}

auto editing_operation(FuzzStream& stream, Modifier& modifier,
                       const FuzzLimits& limits) -> void {
    switch (fuzz_small_int(stream, 0, 5)) {
        case 0:
            add_wire_segment(stream, modifier);
            return;
        case 1:
            delete_temporary_wire_segment(stream, modifier);
            return;
        case 2:
            change_wire_insertion_mode(stream, modifier);
            return;
        case 3:
            move_temporary_wire_unchecked(stream, modifier, limits);
            return;
        case 4:
            move_or_delete_temporary_wire(stream, modifier, limits);
            return;
        case 5:
            set_visible_selection(stream, modifier);
            return;
    }
    std::terminate();
}

auto validate_undo(Modifier& modifier,
                   const std::vector<layout_key_state_t>& key_state_stack) {
    for (const auto& state : std::ranges::views::reverse(key_state_stack)  //
                                 | std::ranges::views::drop(1)) {
        Expects(modifier.has_undo());
        modifier.undo_group();
        Expects(layout_key_state_t {modifier} == state);
    }
    Expects(!modifier.has_undo());
}

auto validate_redo(Modifier& modifier,
                   const std::vector<layout_key_state_t>& key_state_stack) {
    for (const auto& state : key_state_stack | std::ranges::views::drop(1)) {
        Expects(modifier.has_redo());
        modifier.redo_group();
        Expects(layout_key_state_t {modifier} == state);
    }
    Expects(!modifier.has_redo());
}

auto validate_undo_redo(Modifier& modifier,
                        const std::vector<layout_key_state_t>& key_state_stack) {
    Expects(key_state_stack.empty() ||
            layout_key_state_t {modifier} == key_state_stack.back());
    Expects(!modifier.has_ungrouped_undo_entries());

    // Do it twice, as redo may generate different stack entries than the initial entries
    validate_undo(modifier, key_state_stack);
    validate_redo(modifier, key_state_stack);
    validate_undo(modifier, key_state_stack);
    validate_redo(modifier, key_state_stack);
}

auto history_finish_undo_group(Modifier& modifier,
                               std::vector<layout_key_state_t>& key_state_stack) {
    if (modifier.has_ungrouped_undo_entries()) {
        modifier.finish_undo_group();
        key_state_stack.emplace_back(modifier);
    }
}

auto process_data(std::span<const uint8_t> data) -> void {
    auto stream = FuzzStream(data);
    const auto limits = FuzzLimits {};

    auto modifier = Modifier {Layout {}, ModifierConfig {
                                             .enable_history = true,
                                             .validate_messages = true,
                                         }};
    auto key_state_stack = std::vector<layout_key_state_t> {
        layout_key_state_t {modifier},
    };

    while (!stream.empty()) {
        editing_operation(stream, modifier, limits);
        Expects(all_within_limits(modifier.circuit_data().layout, limits));

        if (fuzz_bool(stream)) {
            history_finish_undo_group(modifier, key_state_stack);
        }
    }
    history_finish_undo_group(modifier, key_state_stack);

    if (true) {
        validate_undo_redo(modifier, key_state_stack);
    }
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
