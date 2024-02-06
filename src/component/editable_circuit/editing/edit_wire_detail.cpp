#include "component/editable_circuit/editing/edit_wire_detail.h"

#include "component/editable_circuit/circuit_data.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

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

auto _move_full_segment_between_trees(CircuitData& circuit, segment_t& source_segment,
                                      const wire_id_t destination_id) {
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

namespace move_segment {

auto copy_segment(CircuitData& circuit, const segment_part_t source_segment_part,
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

auto shrink_segment_begin(CircuitData& circuit, const segment_t segment) -> void {
    using namespace info_message;

    if (is_inserted(segment.wire_id)) {
        circuit.submit(SegmentUninserted({
            .segment = segment,
            .segment_info = get_segment_info(circuit.layout, segment),
        }));
    }
}

auto shrink_segment_end(CircuitData& circuit, const segment_t segment,
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

auto _move_touching_segment_between_trees(CircuitData& circuit,
                                          segment_part_t& source_segment_part,
                                          const wire_id_t destination_id) {
    const auto full_part = to_part(get_line(circuit.layout, source_segment_part.segment));
    const auto part_kept =
        difference_touching_one_side(full_part, source_segment_part.part);

    // move
    move_segment::shrink_segment_begin(circuit, source_segment_part.segment);
    const auto destination_segment_part =
        move_segment::copy_segment(circuit, source_segment_part, destination_id);
    const auto leftover_segment_part =
        move_segment::shrink_segment_end(circuit, source_segment_part.segment, part_kept);

    // messages
    circuit.submit(info_message::SegmentPartMoved {
        .destination = destination_segment_part,
        .source = source_segment_part,
    });

    if (part_kept.begin != full_part.begin) {
        circuit.submit(info_message::SegmentPartMoved {
            .destination = leftover_segment_part,
            .source = segment_part_t {.segment = source_segment_part.segment,
                                      .part = part_kept},
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

    source_segment_part = destination_segment_part;
}

auto _move_splitting_segment_between_trees(CircuitData& circuit,
                                           segment_part_t& source_segment_part,
                                           const wire_id_t destination_id) {
    const auto full_part = to_part(get_line(circuit.layout, source_segment_part.segment));
    const auto [part0, part1] =
        difference_not_touching(full_part, source_segment_part.part);

    // move
    const auto source_part1 = segment_part_t {source_segment_part.segment, part1};

    move_segment::shrink_segment_begin(circuit, source_segment_part.segment);
    const auto destination_part1 =
        move_segment::copy_segment(circuit, source_part1, source_part1.segment.wire_id);
    const auto destination_segment_part =
        move_segment::copy_segment(circuit, source_segment_part, destination_id);
    const auto leftover_segment_part =
        move_segment::shrink_segment_end(circuit, source_segment_part.segment, part0);

    // messages
    circuit.submit(info_message::SegmentPartMoved {
        .destination = destination_part1,
        .source = source_part1,
    });

    circuit.submit(info_message::SegmentPartMoved {
        .destination = destination_segment_part,
        .source = source_segment_part,
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

    source_segment_part = destination_segment_part;
}

}  // namespace

auto move_segment_between_trees(CircuitData& circuit, segment_part_t& segment_part,
                                const wire_id_t destination_id) -> void {
    const auto moving_part = segment_part.part;
    const auto full_line = get_line(circuit.layout, segment_part.segment);
    const auto full_part = to_part(full_line);

    if (a_equal_b(moving_part, full_part)) {
        _move_full_segment_between_trees(circuit, segment_part.segment, destination_id);
    } else if (a_inside_b_touching_one_side(moving_part, full_part)) {
        _move_touching_segment_between_trees(circuit, segment_part, destination_id);
    } else if (a_inside_b_not_touching(moving_part, full_part)) {
        _move_splitting_segment_between_trees(circuit, segment_part, destination_id);
    } else {
        throw std::runtime_error("segment part is invalid");
    }
}

//
// Remove Segment from Tree
//

namespace {

auto _remove_full_segment_from_tree(CircuitData& circuit,
                                    segment_part_t& full_segment_part) {
    const auto wire_id = full_segment_part.segment.wire_id;
    const auto segment_index = full_segment_part.segment.segment_index;
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);

    // delete
    const auto last_index = m_tree.last_index();
    m_tree.swap_and_delete_segment(segment_index);

    // messages
    circuit.submit(info_message::SegmentPartDeleted {full_segment_part});

    if (last_index != segment_index) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_t {wire_id, segment_index},
            .old_segment = segment_t {wire_id, last_index},
        });
    }

    full_segment_part = null_segment_part;
}

auto _remove_touching_segment_from_tree(CircuitData& circuit,
                                        segment_part_t& segment_part) {
    const auto wire_id = segment_part.segment.wire_id;
    const auto index = segment_part.segment.segment_index;
    const auto part = segment_part.part;

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);

    const auto full_part = m_tree.part(index);
    const auto part_kept = difference_touching_one_side(full_part, part);

    // delete
    m_tree.shrink_segment(index, part_kept);

    // messages
    circuit.submit(info_message::SegmentPartDeleted {segment_part});

    if (part_kept.begin != full_part.begin) {
        circuit.submit(info_message::SegmentPartMoved {
            .destination = segment_part_t {.segment = segment_part.segment,
                                           .part = m_tree.part(index)},
            .source = segment_part_t {.segment = segment_part.segment, .part = part_kept},
        });
    }

    segment_part = null_segment_part;
}

auto _remove_splitting_segment_from_tree(CircuitData& circuit,
                                         segment_part_t& segment_part) {
    const auto wire_id = segment_part.segment.wire_id;
    const auto index = segment_part.segment.segment_index;
    const auto part = segment_part.part;

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);

    const auto full_part = m_tree.part(index);
    const auto [part0, part1] = difference_not_touching(full_part, part);

    // delete
    const auto index1 = m_tree.copy_segment(m_tree, index, part1);
    m_tree.shrink_segment(index, part0);

    // messages
    const auto segment_part_1 =
        segment_part_t {segment_t {wire_id, index1}, m_tree.part(index1)};

    circuit.submit(info_message::SegmentPartMoved {
        .destination = segment_part_1,
        .source = segment_part_t {segment_part.segment, part1}});

    circuit.submit(info_message::SegmentPartDeleted {segment_part});

    segment_part = null_segment_part;
}

// TODO does this only work for temporary wires ?

}  // namespace

auto remove_segment_from_tree(CircuitData& circuit, segment_part_t& segment_part)
    -> void {
    if (is_inserted(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only remove from non-inserted segments");
    }

    const auto removed_part = segment_part.part;
    const auto full_line = get_line(circuit.layout, segment_part.segment);
    const auto full_part = to_part(full_line);

    if (a_equal_b(removed_part, full_part)) {
        _remove_full_segment_from_tree(circuit, segment_part);
    } else if (a_inside_b_touching_one_side(removed_part, full_part)) {
        _remove_touching_segment_from_tree(circuit, segment_part);
    } else if (a_inside_b_not_touching(removed_part, full_part)) {
        _remove_splitting_segment_from_tree(circuit, segment_part);
    } else {
        throw std::runtime_error("segment part is invalid");
    }
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
