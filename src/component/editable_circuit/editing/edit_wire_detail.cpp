#include "component/editable_circuit/editing/edit_wire_detail.h"

#include "component/editable_circuit/circuit_data.h"
#include "tree_normalization.h"

namespace logicsim {

namespace editable_circuit {

namespace editing {

namespace {

//
// Segment Operations
//

//
// Move Segment Between Tree
//

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

auto __copy_segment(CircuitData& circuit, const segment_part_t source_segment_part,
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

auto __shrink_segment_begin(CircuitData& circuit, const segment_t segment) -> void {
    using namespace info_message;

    if (is_inserted(segment.wire_id)) {
        circuit.submit(SegmentUninserted({
            .segment = segment,
            .segment_info = get_segment_info(circuit.layout, segment),
        }));
    }
}

auto __shrink_segment_end(CircuitData& circuit, const segment_t segment,
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
    move_segment::__shrink_segment_begin(circuit, source_segment_part.segment);
    const auto destination_segment_part =
        move_segment::__copy_segment(circuit, source_segment_part, destination_id);
    const auto leftover_segment_part = move_segment::__shrink_segment_end(
        circuit, source_segment_part.segment, part_kept);

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

    move_segment::__shrink_segment_begin(circuit, source_segment_part.segment);
    const auto destination_part1 =
        move_segment::__copy_segment(circuit, source_part1, source_part1.segment.wire_id);
    const auto destination_segment_part =
        move_segment::__copy_segment(circuit, source_segment_part, destination_id);
    const auto leftover_segment_part =
        move_segment::__shrink_segment_end(circuit, source_segment_part.segment, part0);

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

}  // namespace

// TODO does this only work for temporary wires ?
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

auto split_line_segment(CircuitData& circuit, const segment_t segment,
                        const point_t position) -> segment_part_t {
    const auto full_line = get_line(circuit.layout, segment);
    const auto line_moved = ordered_line_t {position, full_line.p1};

    auto move_segment_part = segment_part_t {segment, to_part(full_line, line_moved)};
    move_segment_between_trees(circuit, move_segment_part, segment.wire_id);

    return move_segment_part;
}

namespace {

auto _merge_line_segments_ordered(CircuitData& circuit, const segment_t segment_0,
                                  const segment_t segment_1,
                                  segment_part_t* preserve_segment) -> void {
    if (segment_0.wire_id != segment_1.wire_id) [[unlikely]] {
        throw std::runtime_error("Cannot merge segments of different trees.");
    }
    if (segment_0.segment_index >= segment_1.segment_index) [[unlikely]] {
        throw std::runtime_error("Segment indices need to be ordered and not the same.");
    }
    const auto is_inserted = ::logicsim::is_inserted(segment_0.wire_id);

    const auto index_0 = segment_0.segment_index;
    const auto index_1 = segment_1.segment_index;
    const auto wire_id = segment_0.wire_id;

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);
    const auto index_last = m_tree.last_index();
    const auto segment_last = segment_t {wire_id, index_last};

    const auto info_0 = m_tree.info(index_0);
    const auto info_1 = m_tree.info(index_1);

    // merge
    m_tree.swap_and_merge_segment({.index_merge_to = index_0, .index_deleted = index_1});
    const auto info_merged = m_tree.info(index_0);

    // messages
    if (is_inserted) {
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
        });
    }

    circuit.submit(info_message::SegmentPartMoved {
        .destination =
            segment_part_t {
                segment_0,
                to_part(info_merged.line, info_1.line),
            },
        .source = segment_part_t {segment_1, to_part(info_1.line)},
    });

    if (is_inserted) {
        circuit.submit(info_message::SegmentInserted {segment_0, info_merged});
    }

    if (index_1 != index_last) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = segment_1,
            .old_segment = segment_last,
        });
        if (is_inserted) {
            circuit.submit(info_message::InsertedSegmentIdUpdated {
                .new_segment = segment_1,
                .old_segment = segment_last,
                .segment_info = m_tree.info(index_1),
            });
        }
    }

    // preserve
    if (preserve_segment && preserve_segment->segment.wire_id == wire_id) {
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
}

}  // namespace

auto merge_line_segments(CircuitData& circuit, segment_t segment_0, segment_t segment_1,
                         segment_part_t* preserve_segment) -> void {
    if (segment_0.segment_index < segment_1.segment_index) {
        _merge_line_segments_ordered(circuit, segment_0, segment_1, preserve_segment);
    } else {
        _merge_line_segments_ordered(circuit, segment_1, segment_0, preserve_segment);
    }
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
        if (mask[segment_index.value]) {
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
        throw std::runtime_error("source is deleted and should have larget id");
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

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
