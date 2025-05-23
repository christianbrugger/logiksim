#include "core/component/editable_circuit/editing/edit_wire_detail.h"

#include "core/component/editable_circuit/circuit_data.h"
#include "core/geometry/line.h"
#include "core/geometry/offset.h"
#include "core/geometry/orientation.h"
#include "core/geometry/part_selections.h"
#include "core/geometry/segment_info.h"
#include "core/tree_normalization.h"
#include "core/vocabulary/endpoints.h"

#include <fmt/core.h>

#include <algorithm>
#include <stdexcept>

namespace logicsim {

namespace editable_circuit {

namespace editing {

//
// Segment Operations
//

auto add_temporary_segment(CircuitData& circuit, ordered_line_t line) -> segment_part_t {
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(temporary_wire_id);

    const auto segment_info = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };

    const auto segment_index = m_tree.add_segment(segment_info);
    const auto segment = segment_t {temporary_wire_id, segment_index};
    const auto part = to_part(segment_info.line);

    // messages
    Expects(part.begin == offset_t {0});
    circuit.submit(info_message::SegmentCreated {
        .segment = segment,
        .size = part.end,
    });

    return segment_part_t {segment, part};
}

//
// Move Segment Between Tree
//

namespace {

// assuming segment already moved
auto _notify_segment_id_changed(CircuitData& circuit, const segment_t source_segment,
                                const segment_t destination_segment,
                                const segment_t last_segment) {
    const auto source_inserted = is_inserted(source_segment.wire_id);
    const auto destination_inserted = is_inserted(destination_segment.wire_id);

    const auto info = get_segment_info(circuit.layout, destination_segment);

    if (source_inserted && !destination_inserted) {
        circuit.submit(info_message::SegmentUninserted({
            .segment = source_segment,
            .segment_info = info,
        }));
    }

    circuit.submit(info_message::SegmentIdUpdated {
        .new_segment = destination_segment,
        .old_segment = source_segment,
    });

    if (source_inserted && destination_inserted) {
        circuit.submit(info_message::InsertedSegmentIdUpdated({
            .new_segment = destination_segment,
            .old_segment = source_segment,
            .segment_info = info,
        }));
    }
    if (destination_inserted && !source_inserted) {
        circuit.submit(info_message::SegmentInserted({
            .segment = destination_segment,
            .segment_info = info,
        }));
    }

    // another element swapped
    if (last_segment != source_segment) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = source_segment,
            .old_segment = last_segment,
        });
    }
    if (last_segment != source_segment && source_inserted) {
        circuit.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = source_segment,
            .old_segment = last_segment,
            .segment_info = get_segment_info(circuit.layout, source_segment),
        });
    }
}

}  // namespace

auto move_full_segment_between_trees(CircuitData& circuit, segment_t& source_segment,
                                     const wire_id_t destination_id) -> void {
    if (source_segment.wire_id == destination_id) {
        return;
    }
    const auto source_index = source_segment.segment_index;

    auto& m_tree_source =
        circuit.layout.wires().modifiable_segment_tree(source_segment.wire_id);
    auto& m_tree_destination =
        circuit.layout.wires().modifiable_segment_tree(destination_id);

    // copy
    const auto destination_index =
        m_tree_destination.copy_segment(m_tree_source, source_index);
    const auto last_index = m_tree_source.last_index();
    m_tree_source.swap_and_delete_segment(source_index);

    // messages
    const auto destination_segment = segment_t {destination_id, destination_index};
    const auto last_segment = segment_t {source_segment.wire_id, last_index};

    _notify_segment_id_changed(circuit, source_segment, destination_segment,
                               last_segment);

    source_segment = destination_segment;
}

namespace {

namespace move_segment {

auto _copy_segment(CircuitData& circuit, const segment_part_t source_segment_part,
                   const wire_id_t destination_id) -> segment_part_t {
    auto& m_tree_source = circuit.layout.wires().modifiable_segment_tree(
        source_segment_part.segment.wire_id);
    auto& m_tree_destination =
        circuit.layout.wires().modifiable_segment_tree(destination_id);

    bool set_input_p0 = false;
    bool set_input_p1 = false;
    // handle inputs being copied within the same tree
    if (destination_id == source_segment_part.segment.wire_id) {
        auto info = m_tree_source.info(source_segment_part.segment.segment_index);
        const auto full_part = to_part(info.line);

        if (full_part.begin == source_segment_part.part.begin &&
            info.p0_type == SegmentPointType::input) {
            info.p0_type = SegmentPointType::shadow_point;
            m_tree_source.update_segment(source_segment_part.segment.segment_index, info);
            set_input_p0 = true;
        }
        if (full_part.end == source_segment_part.part.end &&
            info.p1_type == SegmentPointType::input) {
            info.p1_type = SegmentPointType::shadow_point;
            m_tree_source.update_segment(source_segment_part.segment.segment_index, info);
            set_input_p1 = true;
        }
    }

    const auto destination_index = m_tree_destination.copy_segment(
        m_tree_source, source_segment_part.segment.segment_index,
        source_segment_part.part);

    const auto destination_segment_part =
        segment_part_t {segment_t {destination_id, destination_index},
                        m_tree_destination.part(destination_index)};

    {
        if (set_input_p0) {
            auto info = m_tree_destination.info(destination_index);
            info.p0_type = SegmentPointType::input;
            m_tree_destination.update_segment(destination_index, info);
        }
        if (set_input_p1) {
            auto info = m_tree_destination.info(destination_index);
            info.p1_type = SegmentPointType::input;
            m_tree_destination.update_segment(destination_index, info);
        }
    }

    return destination_segment_part;
}

auto _shrink_segment_begin(CircuitData& circuit, const segment_t segment) -> void {
    using namespace info_message;

    if (is_inserted(segment.wire_id)) {
        circuit.submit(SegmentUninserted({
            .segment = segment,
            .segment_info = get_segment_info(circuit.layout, segment),
        }));
    }
}

auto _shrink_segment_end(CircuitData& circuit, const segment_t segment,
                         const part_t part_kept) -> segment_part_t {
    using namespace info_message;
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
    m_tree.shrink_segment(segment.segment_index, part_kept);

    return segment_part_t {
        .segment = segment,
        .part = m_tree.part(segment.segment_index),
    };
}

}  // namespace move_segment

}  // namespace

auto move_touching_segment_between_trees(
    CircuitData& circuit, segment_part_t& source_segment_part,
    const wire_id_t destination_id,
    segment_key_t optional_end_key) -> move_touching_result_t {
    const auto full_part = to_part(get_line(circuit.layout, source_segment_part.segment));
    const auto part_kept =
        difference_touching_one_side(full_part, source_segment_part.part);
    const auto leftover_at_end = part_kept.begin != full_part.begin;

    // move
    move_segment::_shrink_segment_begin(circuit, source_segment_part.segment);
    const auto destination_segment_part =
        move_segment::_copy_segment(circuit, source_segment_part, destination_id);
    const auto leftover_segment_part = move_segment::_shrink_segment_end(
        circuit, source_segment_part.segment, part_kept);

    // messages
    circuit.submit(info_message::SegmentPartMoved {
        .destination = destination_segment_part,
        .source = source_segment_part,
        .create_destination = true,
        .delete_source = false,
    });

    if (leftover_at_end) {
        circuit.submit(info_message::SegmentPartMoved {
            .destination = leftover_segment_part,
            .source = segment_part_t {.segment = source_segment_part.segment,
                                      .part = part_kept},
            .create_destination = false,
            .delete_source = false,
        });
    }

    if (is_inserted(leftover_segment_part.segment.wire_id)) {
        circuit.submit(info_message::SegmentInserted({
            .segment = leftover_segment_part.segment,
            .segment_info =
                get_segment_info(circuit.layout, leftover_segment_part.segment),
        }));
    }
    if (is_inserted(destination_id)) {
        circuit.submit(info_message::SegmentInserted({
            .segment = destination_segment_part.segment,
            .segment_info =
                get_segment_info(circuit.layout, destination_segment_part.segment),
        }));
    }

    // result
    const auto result = move_touching_result_t {
        .moved = destination_segment_part,
        .other = leftover_segment_part,
        .begin = leftover_at_end ? destination_segment_part : leftover_segment_part,
        .end = leftover_at_end ? leftover_segment_part : destination_segment_part,
    };

    // keys
    if (leftover_at_end) {
        circuit.index.swap_key(result.moved.segment, result.other.segment);
    }
    if (optional_end_key) {
        circuit.index.set_key(result.end.segment, optional_end_key);
    }

    source_segment_part = result.moved;
    Ensures(result.moved != result.other);
    assert(get_line(circuit.layout, result.begin) < get_line(circuit.layout, result.end));
    return result;
}

auto move_splitting_segment_between_trees(
    CircuitData& circuit, segment_part_t& source_segment_part,
    const wire_id_t destination_id,
    move_splitting_keys_t optional_keys) -> move_splitting_result_t {
    const auto full_part = to_part(get_line(circuit.layout, source_segment_part.segment));
    const auto [part0, part1] =
        difference_not_touching(full_part, source_segment_part.part);

    // Two new parts are created:
    //
    // leftover_segment_part | source_segment_part       | source_part_1
    // leftover_segment_part | destination_segment_part  | destination_part_1
    //
    // Note, key is preserved for left_over_segment_part. It is the smallest
    // line / segment, so no adjustment is needed.

    // move
    const auto source_part1 = segment_part_t {source_segment_part.segment, part1};

    move_segment::_shrink_segment_begin(circuit, source_segment_part.segment);
    const auto destination_part1 =
        move_segment::_copy_segment(circuit, source_part1, source_part1.segment.wire_id);
    const auto destination_segment_part =
        move_segment::_copy_segment(circuit, source_segment_part, destination_id);
    const auto leftover_segment_part =
        move_segment::_shrink_segment_end(circuit, source_segment_part.segment, part0);

    // messages
    circuit.submit(info_message::SegmentPartMoved {
        .destination = destination_part1,
        .source = source_part1,
        .create_destination = true,
        .delete_source = false,
    });

    circuit.submit(info_message::SegmentPartMoved {
        .destination = destination_segment_part,
        .source = source_segment_part,
        .create_destination = true,
        .delete_source = false,
    });

    if (is_inserted(leftover_segment_part.segment.wire_id)) {
        circuit.submit(info_message::SegmentInserted({
            .segment = leftover_segment_part.segment,
            .segment_info =
                get_segment_info(circuit.layout, leftover_segment_part.segment),
        }));
    }
    if (is_inserted(destination_part1.segment.wire_id)) {
        circuit.submit(info_message::SegmentInserted({
            .segment = destination_part1.segment,
            .segment_info = get_segment_info(circuit.layout, destination_part1.segment),
        }));
    }
    if (is_inserted(destination_segment_part.segment.wire_id)) {
        circuit.submit(info_message::SegmentInserted({
            .segment = destination_segment_part.segment,
            .segment_info =
                get_segment_info(circuit.layout, destination_segment_part.segment),
        }));
    }

    const auto result = move_splitting_result_t {
        .begin = leftover_segment_part,
        .middle = destination_segment_part,
        .end = destination_part1,
    };

    // keys
    if (optional_keys.new_middle_key) {
        circuit.index.set_key(result.middle.segment, optional_keys.new_middle_key);
    }
    if (optional_keys.new_end_key) {
        circuit.index.set_key(result.end.segment, optional_keys.new_end_key);
    }

    source_segment_part = result.middle;
    assert(get_line(circuit.layout, result.begin) <
           get_line(circuit.layout, result.middle));
    assert(get_line(circuit.layout, result.middle) <
           get_line(circuit.layout, result.end));
    return result;
}

auto move_touching_result_t::format() const -> std::string {
    return fmt::format(
        "move_touching_result_t{{moved_part = {}, other_part = {}, "
        "begin_part = {}, end_part = {}}}",
        moved, other, begin, end);
}

auto move_splitting_keys_t::format() const -> std::string {
    return fmt::format("move_splitting_keys_t{{new_middle_key = {}, new_end_key = {}}}",
                       new_middle_key, new_end_key);
}

auto move_splitting_result_t::format() const -> std::string {
    return fmt::format(
        "move_splitting_result_t{{begin_part = {}, middle_part = {}, end_part = {}}}",
        begin, middle, end);
}

}  // namespace editing
}  // namespace editable_circuit

template <>
auto format(editable_circuit::editing::move_segment_type type) -> std::string {
    switch (type) {
        using enum editable_circuit::editing::move_segment_type;

        case move_full_segment:
            return "move_full_segment";
        case move_touching_segment:
            return "move_touching_segment";
        case move_splitting_segment:
            return "move_splitting_segment";
    };
    std::terminate();
}

namespace editable_circuit {
namespace editing {

auto get_move_segment_type(const Layout& layout,
                           segment_part_t segment_part) -> move_segment_type {
    const auto moving_part = segment_part.part;
    const auto full_part = get_part(layout, segment_part.segment);

    using enum move_segment_type;

    if (a_equal_b(moving_part, full_part)) {
        return move_full_segment;
    }

    if (a_inside_b_touching_one_side(moving_part, full_part)) {
        return move_touching_segment;
    };

    if (a_inside_b_not_touching(moving_part, full_part)) {
        return move_splitting_segment;
    }

    throw std::runtime_error("segment part is invalid");
}

auto move_segment_between_trees(CircuitData& circuit, segment_part_t& segment_part,
                                const wire_id_t destination_id) -> void {
    switch (get_move_segment_type(circuit.layout, segment_part)) {
        using enum move_segment_type;

        case move_full_segment: {
            move_full_segment_between_trees(circuit, segment_part.segment,
                                            destination_id);
            return;
        }

        case move_touching_segment: {
            move_touching_segment_between_trees(circuit, segment_part, destination_id);
            return;
        }
        case move_splitting_segment: {
            move_splitting_segment_between_trees(circuit, segment_part, destination_id);
            return;
        }
    }
}

auto move_touching_segment_between_trees_with_history(
    CircuitData& circuit, segment_part_t& source_segment_part, wire_id_t destination_id,
    segment_key_t optional_end_key) -> move_touching_result_t {
    const auto result = move_touching_segment_between_trees(
        circuit, source_segment_part, destination_id, optional_end_key);

    if (const auto stack = circuit.history.get_stack()) {
        const auto key_begin = circuit.index.key_index().get(result.begin.segment);
        const auto key_end = circuit.index.key_index().get(result.end.segment);

        stack->push_segment_merge(key_begin, key_end);
    }

    return result;
}

auto move_splitting_segment_between_trees_with_history(
    CircuitData& circuit, segment_part_t& source_segment_part, wire_id_t destination_id,
    move_splitting_keys_t optional_keys) -> move_splitting_result_t {
    const auto result = move_splitting_segment_between_trees(
        circuit, source_segment_part, destination_id, optional_keys);

    if (const auto stack = circuit.history.get_stack()) {
        const auto key_begin = circuit.index.key_index().get(result.begin.segment);
        const auto key_middle = circuit.index.key_index().get(result.middle.segment);
        const auto key_end = circuit.index.key_index().get(result.end.segment);

        // restored in reverse order
        stack->push_segment_merge(key_begin, key_middle);
        stack->push_segment_merge(key_middle, key_end);
    }

    return result;
}

auto move_segment_between_trees_with_history(CircuitData& circuit,
                                             segment_part_t& segment_part,
                                             const wire_id_t destination_id) -> void {
    switch (get_move_segment_type(circuit.layout, segment_part)) {
        using enum move_segment_type;

        case move_full_segment: {
            move_full_segment_between_trees(circuit, segment_part.segment,
                                            destination_id);
            return;
        }

        case move_touching_segment: {
            move_touching_segment_between_trees_with_history(circuit, segment_part,
                                                             destination_id);
            return;
        }
        case move_splitting_segment: {
            move_splitting_segment_between_trees_with_history(circuit, segment_part,
                                                              destination_id);
            return;
        }
    }
}

//
// Remove Segment from Tree
//

auto remove_full_segment_from_uninserted_tree(CircuitData& circuit,
                                              segment_part_t& full_segment_part) -> void {
    if (is_inserted(full_segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only remove from non-inserted segments");
    }
    if (!is_full_segment(circuit.layout, full_segment_part)) [[unlikely]] {
        throw std::runtime_error("can only delete full segments");
    }

    const auto wire_id = full_segment_part.segment.wire_id;
    const auto segment_index = full_segment_part.segment.segment_index;
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);

    // delete
    const auto last_index = m_tree.last_index();
    m_tree.swap_and_delete_segment(segment_index);

    // messages
    circuit.submit(info_message::SegmentPartDeleted {
        .segment_part = full_segment_part,
        .delete_segment = true,
    });

    if (last_index != segment_index) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_t {wire_id, segment_index},
            .old_segment = segment_t {wire_id, last_index},
        });
    }

    full_segment_part = null_segment_part;
}

auto split_line_segment(CircuitData& circuit, const segment_t segment,
                        const point_t position) -> segment_part_t {
    const auto full_line = get_line(circuit.layout, segment);
    const auto line_moved = ordered_line_t {position, full_line.p1};

    auto move_segment_part = segment_part_t {segment, to_part(full_line, line_moved)};
    move_segment_between_trees(circuit, move_segment_part, segment.wire_id);

    return move_segment_part;
}

namespace {

/**
 * @brief: Mergest segment_1 into segment_1
 */
auto _merge_line_segments_ordered(CircuitData& circuit, const segment_t segment_0,
                                  const segment_t segment_1,
                                  segment_part_t* preserve_segment) -> segment_t {
    if (segment_0.wire_id != segment_1.wire_id) [[unlikely]] {
        throw std::runtime_error("Cannot merge segments of different trees.");
    }
    if (segment_0.segment_index >= segment_1.segment_index) [[unlikely]] {
        throw std::runtime_error("Segment indices need to be ordered and not the same.");
    }
    const auto was_inserted = is_inserted(segment_0.wire_id);

    const auto index_0 = segment_0.segment_index;
    const auto index_1 = segment_1.segment_index;
    const auto wire_id = segment_0.wire_id;

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);
    const auto index_last = m_tree.last_index();
    const auto segment_last = segment_t {wire_id, index_last};

    const auto info_0 = segment_info_t {m_tree.info(index_0)};
    const auto info_1 = segment_info_t {m_tree.info(index_1)};

    const auto merge_key_1 = info_0.line > info_1.line;
    const auto key_1 =
        merge_key_1 ? circuit.index.key_index().get(segment_1) : null_segment_key;

    // merge
    m_tree.swap_and_merge_segment({.index_merge_to = index_0, .index_deleted = index_1});
    const auto& info_merged = m_tree.info(index_0);

    // messages
    if (was_inserted) {
        circuit.submit(info_message::SegmentUninserted {segment_0, info_0});
        circuit.submit(info_message::SegmentUninserted {segment_1, info_1});
    }

    if (to_part(info_0.line) != to_part(info_merged.line, info_0.line)) {
        circuit.submit(info_message::SegmentPartMoved {
            .destination =
                segment_part_t {
                    segment_0,
                    to_part(info_merged.line, info_0.line),
                },
            .source = segment_part_t {segment_0, to_part(info_0.line)},
            .create_destination = false,
            .delete_source = false,
        });
    }

    circuit.submit(info_message::SegmentPartMoved {
        .destination =
            segment_part_t {
                segment_0,
                to_part(info_merged.line, info_1.line),
            },
        .source = segment_part_t {segment_1, to_part(info_1.line)},
        .create_destination = false,
        .delete_source = true,
    });

    if (was_inserted) {
        circuit.submit(info_message::SegmentInserted {segment_0, info_merged});
    }

    if (index_1 != index_last) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
        });
        if (was_inserted) {
            circuit.submit(info_message::InsertedSegmentIdUpdated {
                .new_segment = segment_1,
                .old_segment = segment_last,
                .segment_info = m_tree.info(index_1),
            });
        }
    }

    // update key
    if (merge_key_1) {
        circuit.index.set_key(segment_0, key_1);
    }

    // preserve
    if (preserve_segment != nullptr && preserve_segment->segment.wire_id == wire_id) {
        const auto p_index = preserve_segment->segment.segment_index;

        if (p_index == index_0 || p_index == index_1) {
            const auto p_info = p_index == index_0 ? info_0 : info_1;
            const auto p_line = to_line(p_info.line, preserve_segment->part);
            const auto p_part = to_part(info_merged.line, p_line);
            *preserve_segment = segment_part_t {segment_t {wire_id, index_0}, p_part};
        }

        else if (p_index == index_last) {
            const auto p_part = preserve_segment->part;
            *preserve_segment = segment_part_t {segment_t {wire_id, index_1}, p_part};
        }
    }

    return segment_0;
}

}  // namespace

auto are_segments_mergeable(const Layout& layout, segment_t segment_0,
                            segment_t segment_1) -> bool {
    if (!segment_0 || !segment_1) {
        return false;
    }
    if (segment_0.wire_id != segment_1.wire_id) {
        return false;
    }
    if (segment_0.segment_index == segment_1.segment_index) {
        return false;
    }

    const auto [line_0, line_1] =
        sorted(get_line(layout, segment_0), get_line(layout, segment_1));

    return is_horizontal(line_0) == is_horizontal(line_1) &&  //
           line_0.p1 == line_1.p0;
}

auto merge_line_segment(CircuitData& circuit, segment_t segment_0, segment_t segment_1,
                        segment_part_t* preserve_segment) -> segment_t {
    if (!are_segments_mergeable(circuit.layout, segment_0, segment_1)) [[unlikely]] {
        throw std::runtime_error("segments are not mergeable");
    }

    if (segment_0.segment_index < segment_1.segment_index) {
        return _merge_line_segments_ordered(circuit, segment_0, segment_1,
                                            preserve_segment);
    }
    return _merge_line_segments_ordered(circuit, segment_1, segment_0, preserve_segment);
}

namespace {

struct merge_memory_t {
    segment_key_t key_0 {null_segment_key};
    segment_key_t key_1 {null_segment_key};
    segment_info_t info_0 {};
    segment_info_t info_1 {};
};

[[nodiscard]] auto sort_memory(const merge_memory_t& memory) -> merge_memory_t {
    if (memory.info_0.line <= memory.info_1.line) {
        return memory;
    }

    return merge_memory_t {
        .key_0 = memory.key_1,
        .key_1 = memory.key_0,
        .info_0 = memory.info_1,
        .info_1 = memory.info_0,
    };
};

}  // namespace

auto merge_line_segment_with_history(CircuitData& circuit, segment_t segment_0,
                                     segment_t segment_1,
                                     segment_part_t* preserve_segment) -> segment_t {
    const auto stack = circuit.history.get_stack();

    // remember state
    const auto before = [&]() {
        if (stack != nullptr) {
            return sort_memory(merge_memory_t {
                .key_0 = circuit.index.key_index().get(segment_0),
                .key_1 = circuit.index.key_index().get(segment_1),
                .info_0 = get_segment_info(circuit.layout, segment_0),
                .info_1 = get_segment_info(circuit.layout, segment_1),
            });
        }
        return merge_memory_t {};
    }();

    // merge
    const auto segment_after =
        merge_line_segment(circuit, segment_0, segment_1, preserve_segment);

    // history
    if (stack != nullptr) {
        Expects(before.info_0.line <= before.info_1.line);

        if (is_temporary(segment_0.wire_id)) {
            if (before.info_0.p1_type != SegmentPointType::shadow_point) {
                stack->push_segment_set_endpoints(before.key_0,
                                                  get_endpoints(before.info_0));
            }
            if (before.info_1.p0_type != SegmentPointType::shadow_point) {
                stack->push_segment_set_endpoints(before.key_1,
                                                  get_endpoints(before.info_1));
            }
        }

        // merge preserves key of the first line
        const auto key_after = before.key_0;
        assert(key_after == circuit.index.key_index().get(segment_after));

        auto definition = split_segment_key_t {
            .source = key_after,
            .new_key = before.key_1,
            .split_offset = to_offset(before.info_0.line),
        };

        stack->push_segment_split(definition);
    }

    return segment_after;
}

//
// Wire Operations
//

namespace {

auto _notify_wire_id_change(CircuitData& circuit, const wire_id_t new_wire_id,
                            const wire_id_t old_wire_id) {
    const auto& segment_tree = circuit.layout.wires().segment_tree(new_wire_id);

    for (auto&& segment_index : segment_tree.indices()) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_t {new_wire_id, segment_index},
            .old_segment = segment_t {old_wire_id, segment_index},
        });
    }

    if (is_inserted(new_wire_id)) {
        for (auto&& segment_index : segment_tree.indices()) {
            circuit.submit(info_message::InsertedSegmentIdUpdated {
                .new_segment = segment_t {new_wire_id, segment_index},
                .old_segment = segment_t {old_wire_id, segment_index},
                .segment_info = segment_tree.info(segment_index),
            });
        }
    }
}

}  // namespace

auto swap_and_delete_empty_wire(CircuitData& circuit, wire_id_t& wire_id,
                                wire_id_t* preserve_element) -> void {
    if (!wire_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    if (!is_inserted(wire_id)) [[unlikely]] {
        throw std::runtime_error("can only delete inserted wires");
    }
    if (!is_wire_empty(circuit.layout, wire_id)) [[unlikely]] {
        throw std::runtime_error("can't delete wires with segments");
    }

    // delete in underlying
    auto last_id = circuit.layout.wires().swap_and_delete(wire_id);

    if (wire_id != last_id) {
        _notify_wire_id_change(circuit, wire_id, last_id);
    }

    if (preserve_element != nullptr) {
        if (*preserve_element == wire_id) {
            *preserve_element = null_wire_id;
        } else if (*preserve_element == last_id) {
            *preserve_element = wire_id;
        }
    }

    wire_id = null_wire_id;
}

// TODO throw if not broken tree
auto split_broken_tree(CircuitData& circuit, point_t p0, point_t p1) -> wire_id_t {
    const auto p0_tree_id = circuit.index.collision_index().get_first_wire(p0);
    const auto p1_tree_id = circuit.index.collision_index().get_first_wire(p1);

    if (!p0_tree_id || !p1_tree_id || p0_tree_id != p1_tree_id) {
        // throw std::runtime_error("not a broken tree");
        return null_wire_id;
    };

    // create new tree
    const auto new_tree_id = circuit.layout.wires().add_wire();

    // find connected segments
    const auto& tree_from = circuit.layout.wires().modifiable_segment_tree(p0_tree_id);
    const auto mask = calculate_connected_segments_mask(tree_from, p1);

    // move over segments
    for (const auto segment_index : tree_from.indices().reverse()) {
        if (mask.at(size_t {segment_index})) {
            auto segment_part = segment_part_t {segment_t {p0_tree_id, segment_index},
                                                tree_from.part(segment_index)};
            move_segment_between_trees(circuit, segment_part, new_tree_id);
        }
    }

    assert(is_contiguous_tree_with_correct_endpoints(tree_from));
    assert(is_contiguous_tree_with_correct_endpoints(
        circuit.layout.wires().segment_tree(new_tree_id)));

    return new_tree_id;
}

// TODO sort arguments
auto merge_and_delete_tree(CircuitData& circuit, wire_id_t& tree_destination,
                           wire_id_t& tree_source) -> void {
    if (tree_destination >= tree_source) [[unlikely]] {
        // optimization
        throw std::runtime_error("source is deleted and should have larger id");
    }

    if (!is_inserted(tree_source) && !is_inserted(tree_destination)) [[unlikely]] {
        throw std::runtime_error("only supports merging of inserted trees");
    }

    auto& m_tree_source = circuit.layout.wires().modifiable_segment_tree(tree_source);
    auto& m_tree_destination =
        circuit.layout.wires().modifiable_segment_tree(tree_destination);

    auto new_index = m_tree_destination.last_index();

    for (auto old_index : m_tree_source.indices()) {
        const auto segment_info = m_tree_source.info(old_index);
        ++new_index;

        const auto old_segment = segment_t {tree_source, old_index};
        const auto new_segment = segment_t {tree_destination, new_index};

        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = new_segment,
            .old_segment = old_segment,
        });
        circuit.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = new_segment,
            .old_segment = old_segment,
            .segment_info = segment_info,
        });
    }

    m_tree_destination.add_tree(m_tree_source);

    m_tree_source.clear();
    swap_and_delete_empty_wire(circuit, tree_source, &tree_destination);
}

//
// Endpoints
//

auto temporary_endpoints_valid(endpoints_t endpoints) -> bool {
    const auto valid_temporary = [](SegmentPointType type) {
        return type == SegmentPointType::shadow_point ||
               type == SegmentPointType::cross_point;
    };
    return valid_temporary(endpoints.p0_type) && valid_temporary(endpoints.p1_type);
}

namespace {

template <class T>
    requires std::invocable<T, const segment_info_t&> &&
                 std::same_as<segment_info_t,
                              std::invoke_result_t<T, const segment_info_t&>>
auto _set_endpoints(CircuitData& circuit, segment_t segment, T func,
                    HistoryFlag with_history) -> void {
    if (is_colliding(segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("cannot change endpoints for colliding wires");
    }

    const auto old_info = get_segment_info(circuit.layout, segment);
    const auto new_info = std::invoke(func, old_info);

    if (old_info == new_info) {
        return;
    }

    if (is_temporary(segment.wire_id) &&
        !temporary_endpoints_valid(get_endpoints(new_info))) [[unlikely]] {
        throw std::runtime_error(
            "New endpoints needs to be shadow_point or cross_point for temporary wires");
    }

    if (with_history == HistoryFlag::with_history) {
        if (const auto stack = circuit.history.get_stack()) {
            const auto segment_key = circuit.index.key_index().get(segment);
            stack->push_segment_set_endpoints(segment_key, get_endpoints(old_info));
        }
    }

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
    m_tree.update_segment(segment.segment_index, new_info);

    if (is_inserted(segment.wire_id)) {
        circuit.submit(info_message::InsertedEndPointsUpdated {
            .segment = segment,
            .new_segment_info = new_info,
            .old_segment_info = old_info,
        });
    }
}

}  // namespace

auto set_endpoints(CircuitData& circuit, segment_t segment, endpoints_t endpoints,
                   HistoryFlag with_history) -> void {
    const auto to_new_info = [endpoints](const segment_info_t& old_info) {
        return to_segment_info(old_info.line, endpoints);
    };

    _set_endpoints(circuit, segment, to_new_info, with_history);
}

auto set_endpoints(CircuitData& circuit, segment_t segment, point_t position,
                   SegmentPointType type, HistoryFlag with_history) -> void {
    const auto to_new_info = [position, type](const segment_info_t& old_info) {
        return updated_segment_info(old_info, position, type);
    };

    _set_endpoints(circuit, segment, to_new_info, with_history);
}

auto reset_endpoints(CircuitData& circuit, const segment_t segment,
                     HistoryFlag with_history) -> void {
    set_endpoints(circuit, segment,
                  endpoints_t {
                      .p0_type = SegmentPointType::shadow_point,
                      .p1_type = SegmentPointType::shadow_point,
                  },
                  with_history);
}

auto reset_endpoints(CircuitData& circuit, const segment_part_t segment_part,
                     HistoryFlag with_history) -> void {
    const auto to_new_info = [part = segment_part.part](const segment_info_t& old_info) {
        using enum SegmentPointType;
        const auto full_part = to_part(old_info.line);

        const auto new_endpoints = endpoints_t {
            .p0_type = part.begin == full_part.begin ? shadow_point : old_info.p0_type,
            .p1_type = part.end == full_part.end ? shadow_point : old_info.p1_type,
        };

        return to_segment_info(old_info.line, new_endpoints);
    };

    _set_endpoints(circuit, segment_part.segment, to_new_info, with_history);
}

//
// Bulk
//

auto update_inserted_segment_endpoints(CircuitData& circuit, wire_id_t wire_id,
                                       point_update_t data,
                                       const point_t position) -> void {
    if (!is_inserted(wire_id)) [[unlikely]] {
        throw std::runtime_error("only works for inserted segment trees.");
    }

    const auto run_point_update = [&](bool to_shadow) {
        for (auto [segment_index, point_type] : data) {
            const auto segment = segment_t {wire_id, segment_index};
            const auto type = to_shadow ? SegmentPointType::shadow_point : point_type;
            set_endpoints(circuit, segment, position, type);
        }
    };

    // first empty index
    run_point_update(true);
    // write the new states
    run_point_update(false);
}

namespace {

auto _sort_through_lines_first(
    std::span<std::pair<ordered_line_t, segment_index_t>> lines,
    const point_t point) -> void {
    std::ranges::sort(lines, {},
                      [point](std::pair<ordered_line_t, segment_index_t> item) {
                          return is_endpoint(point, item.first);
                      });
}

}  // namespace

auto fix_and_merge_inserted_segments(CircuitData& circuit, const point_t position,
                                     segment_part_t* preserve_segment) -> void {
    const auto segments = circuit.index.selection_index().query_line_segments(position);
    const auto segment_count = get_segment_count(segments);

    if (segment_count == 0) {
        return;
    }
    const auto wire_id = get_unique_wire_id(segments);
    const auto indices = get_segment_indices(segments);

    if (segment_count == 1) {
        const auto new_type = get_segment_point_type(circuit.layout, segments.at(0),
                                                     position) == SegmentPointType::input
                                  ? SegmentPointType::input
                                  : SegmentPointType::output;

        update_inserted_segment_endpoints(circuit, wire_id,
                                          {
                                              std::pair {indices.at(0), new_type},
                                          },
                                          position);

        return;
    }

    if (segment_count == 2) {
        auto lines = std::array {
            std::pair {get_line(circuit.layout, segments.at(0)), indices.at(0)},
            std::pair {get_line(circuit.layout, segments.at(1)), indices.at(1)},
        };
        _sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            split_line_segment(circuit, segment_t {wire_id, lines.at(0).second},
                               position);
            fix_and_merge_inserted_segments(circuit, position, preserve_segment);
            return;
        }

        const auto horizontal_0 = is_horizontal(lines.at(0).first);
        const auto horizontal_1 = is_horizontal(lines.at(1).first);
        const auto parallel = horizontal_0 == horizontal_1;

        if (parallel) {
            merge_line_segment_with_history(circuit, segments.at(0), segments.at(1),
                                            preserve_segment);
            return;
        }

        // this handles corners
        update_inserted_segment_endpoints(
            circuit, wire_id,
            {
                std::pair {indices.at(0), SegmentPointType::corner_point},
                std::pair {indices.at(1), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    if (segment_count == 3) {
        auto lines = std::array {
            std::pair {get_line(circuit.layout, segments.at(0)), indices.at(0)},
            std::pair {get_line(circuit.layout, segments.at(1)), indices.at(1)},
            std::pair {get_line(circuit.layout, segments.at(2)), indices.at(2)},
        };
        _sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) [[unlikely]] {
            throw std::runtime_error("This is not allowed, segment must be split");
        }

        update_inserted_segment_endpoints(
            circuit, wire_id,
            {
                std::pair {indices.at(0), SegmentPointType::cross_point},
                std::pair {indices.at(1), SegmentPointType::shadow_point},
                std::pair {indices.at(2), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    if (segment_count == 4) {
        update_inserted_segment_endpoints(
            circuit, wire_id,
            {
                std::pair {indices.at(0), SegmentPointType::cross_point},
                std::pair {indices.at(1), SegmentPointType::shadow_point},
                std::pair {indices.at(2), SegmentPointType::shadow_point},
                std::pair {indices.at(3), SegmentPointType::shadow_point},
            },
            position);
        return;
    }

    throw std::runtime_error("unexpected unhandled case");
}

//
// Valid
//

auto mark_valid(Layout& layout, const segment_part_t segment_part) -> void {
    auto& m_tree = layout.wires().modifiable_segment_tree(segment_part.segment.wire_id);
    m_tree.mark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto unmark_valid(Layout& layout, const segment_part_t segment_part) -> void {
    auto& m_tree = layout.wires().modifiable_segment_tree(segment_part.segment.wire_id);
    m_tree.unmark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto mark_valid_with_history(CircuitData& circuit,
                             const segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto& valid_parts = get_segment_valid_parts(circuit.layout,  //
                                                          segment_part.segment);

        const auto store_history = [segment = segment_part.segment, stack,
                                    &index = circuit.index.key_index()](part_t part,
                                                                        bool is_valid) {
            if (!is_valid) {
                const auto segment_key = index.get(segment);
                stack->push_segment_colliding_to_insert(segment_key, part);
            }
        };

        iter_parts_partial(segment_part.part, valid_parts, store_history);
    }

    mark_valid(circuit.layout, segment_part);
}

auto unmark_valid_with_history(CircuitData& circuit,
                               const segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto& valid_parts = get_segment_valid_parts(circuit.layout,  //
                                                          segment_part.segment);

        const auto store_history = [segment = segment_part.segment, stack,
                                    &index = circuit.index.key_index()](part_t part,
                                                                        bool is_valid) {
            if (is_valid) {
                const auto segment_key = index.get(segment);
                stack->push_segment_insert_to_colliding_expect_valid(segment_key, part);
            }
        };

        iter_parts_partial(segment_part.part, valid_parts, store_history);
    }

    unmark_valid(circuit.layout, segment_part);
}

//
// Collisions
//

namespace {

auto _wire_endpoints_colliding(const CircuitData& circuit, ordered_line_t line) -> bool {
    const auto wire_id_0 = circuit.index.collision_index().get_first_wire(line.p0);
    const auto wire_id_1 = circuit.index.collision_index().get_first_wire(line.p1);

    // loop check
    if (wire_id_0 && wire_id_0 == wire_id_1) {
        return true;
    }

    // count existing inputs
    auto input_count = 0;
    if (wire_id_0 && circuit.layout.wires().segment_tree(wire_id_0).has_input()) {
        ++input_count;
    }
    if (wire_id_1 && circuit.layout.wires().segment_tree(wire_id_1).has_input()) {
        ++input_count;
    }
    if (input_count > 1) {
        return true;
    }

    // check for LogicItem Outputs  (requires additional inputs)
    if (!wire_id_0) {
        if (const auto entry = circuit.index.logicitem_output_index().find(line.p0)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p0(line))) {
                return true;
            }
            ++input_count;
        }
    }
    if (!wire_id_1) {
        if (const auto entry = circuit.index.logicitem_output_index().find(line.p1)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p1(line))) {
                return true;
            }
            ++input_count;
        }
    }
    if (input_count > 1) {
        return true;
    }

    // check for LogicItem Inputs
    if (!wire_id_0) {
        if (const auto entry = circuit.index.logicitem_input_index().find(line.p0)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p0(line))) {
                return true;
            }
        }
    }
    if (!wire_id_1) {
        if (const auto entry = circuit.index.logicitem_input_index().find(line.p1)) {
            if (!orientations_compatible(entry->orientation, to_orientation_p1(line))) {
                return true;
            }
        }
    }

    return false;
}

}  // namespace

auto is_wire_colliding(const CircuitData& circuit, const ordered_line_t line) -> bool {
    return _wire_endpoints_colliding(circuit, line) ||
           circuit.index.collision_index().is_colliding(line);
}

//
// Connections
//

auto set_wire_inputs_at_logicitem_outputs(CircuitData& circuit,
                                          segment_t segment) -> void {
    if (!is_inserted(segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("wire needs to be inserted");
    }

    const auto line = get_line(circuit.layout, segment);

    // find LogicItem outputs
    if (const auto entry = circuit.index.logicitem_output_index().find(line.p0)) {
        set_endpoints(circuit, segment, line.p0, SegmentPointType::input);
    }
    if (const auto entry = circuit.index.logicitem_output_index().find(line.p1)) {
        set_endpoints(circuit, segment, line.p1, SegmentPointType::input);
    }
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
