#include "component/editable_circuit/edit_wire.h"

#include "component/editable_circuit/circuit_data.h"
#include "geometry/line.h"
#include "geometry/orientation.h"
#include "layout.h"
#include "tree_normalization.h"

#include <algorithm>

namespace logicsim {

namespace editable_circuit {

//
// Move Segment Between Tree
//

namespace {

// segment already moved
auto notify_segment_insertion_status_changed(CircuitData& circuit,
                                             const segment_t source_segment,
                                             const segment_t destination_segment,
                                             const segment_t last_segment) {
    const auto source_inserted = is_inserted(source_segment.wire_id);
    const auto destination_inserted = is_inserted(destination_segment.wire_id);

    const auto info = get_segment_info(circuit.layout, destination_segment);

    // insertion / un-insertion
    if (source_inserted && destination_inserted) {
        circuit.submit(info_message::InsertedSegmentIdUpdated({
            .new_segment = destination_segment,
            .old_segment = source_segment,
            .segment_info = info,
        }));
    }
    if (source_inserted && !destination_inserted) {
        circuit.submit(info_message::SegmentUninserted({
            .segment = source_segment,
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
    if (last_segment != source_segment && source_inserted) {
        circuit.submit(info_message::InsertedSegmentIdUpdated {
            .new_segment = source_segment,
            .old_segment = last_segment,
            .segment_info = get_segment_info(circuit.layout, source_segment),
        });
    }
}

// segment already moved
auto notify_segment_id_changed(CircuitData& circuit, const segment_t source_segment,
                               const segment_t destination_segment,
                               const segment_t last_segment) {
    circuit.submit(info_message::SegmentIdUpdated {
        .new_segment = destination_segment,
        .old_segment = source_segment,
    });

    // another element swapped
    if (last_segment != source_segment) {
        circuit.submit(info_message::SegmentIdUpdated {
            .new_segment = source_segment,
            .old_segment = last_segment,
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

    notify_segment_id_changed(circuit, source_segment, destination_segment, last_segment);
    notify_segment_insertion_status_changed(circuit, source_segment, destination_segment,
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
    {
        if (destination_id == source_segment_part.segment.wire_id) {
            auto info = m_tree_source.info(source_segment_part.segment.segment_index);
            const auto full_part = to_part(info.line);

            if (full_part.begin == source_segment_part.part.begin &&
                info.p0_type == SegmentPointType::input) {
                info.p0_type = SegmentPointType::shadow_point;
                m_tree_source.update_segment(source_segment_part.segment.segment_index,
                                             info);
                set_input_p0 = true;
            }
            if (full_part.end == source_segment_part.part.end &&
                info.p1_type == SegmentPointType::input) {
                info.p1_type = SegmentPointType::shadow_point;
                m_tree_source.update_segment(source_segment_part.segment.segment_index,
                                             info);
                set_input_p1 = true;
            }
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

    circuit.submit(info_message::SegmentCreated {destination_segment_part.segment});

    if (is_inserted(destination_id)) {
        circuit.submit(info_message::SegmentInserted({
            .segment = destination_segment_part.segment,
            .segment_info =
                get_segment_info(circuit.layout, destination_segment_part.segment),
        }));
    }

    return destination_segment_part;
}

auto shrink_segment_begin(CircuitData& circuit, const segment_t segment) -> void {
    using namespace info_message;

    if (is_inserted(segment.wire_id)) {
        auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
        const auto old_info = m_tree.info(segment.segment_index);
        circuit.submit(SegmentUninserted({.segment = segment, .segment_info = old_info}));
    }
}

auto shrink_segment_end(CircuitData& circuit, const segment_t segment,
                        const part_t part_kept) -> segment_part_t {
    using namespace info_message;
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
    m_tree.shrink_segment(segment.segment_index, part_kept);

    if (is_inserted(segment.wire_id)) {
        const auto new_info = m_tree.info(segment.segment_index);
        circuit.submit(SegmentInserted({.segment = segment, .segment_info = new_info}));
    }

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
        .segment_part_destination = destination_segment_part,
        .segment_part_source = source_segment_part,
    });

    if (part_kept.begin != full_part.begin) {
        circuit.submit(info_message::SegmentPartMoved {
            .segment_part_destination = leftover_segment_part,
            .segment_part_source = segment_part_t {.segment = source_segment_part.segment,
                                                   .part = part_kept},
        });
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
    move_segment::shrink_segment_end(circuit, source_segment_part.segment, part0);

    // messages
    circuit.submit(info_message::SegmentPartMoved {
        .segment_part_destination = destination_part1,
        .segment_part_source = source_part1,
    });

    circuit.submit(info_message::SegmentPartMoved {
        .segment_part_destination = destination_segment_part,
        .segment_part_source = source_segment_part,
    });

    source_segment_part = destination_segment_part;
}

//  * trees can become empty
//  * inserts new endpoints as shaddow points
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

}  // namespace

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
            .segment_part_destination = segment_part_t {.segment = segment_part.segment,
                                                        .part = m_tree.part(index)},
            .segment_part_source =
                segment_part_t {.segment = segment_part.segment, .part = part_kept},
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

    circuit.submit(info_message::SegmentCreated {segment_part_1.segment});

    circuit.submit(info_message::SegmentPartMoved {
        .segment_part_destination = segment_part_1,
        .segment_part_source = segment_part_t {segment_part.segment, part1}});

    circuit.submit(info_message::SegmentPartDeleted {segment_part});

    segment_part = null_segment_part;
}

// TODO does this only work for temporary wires ?

/**
 * @brief:
 *
 * trees can become empty
 * inserts new endpoints as shadow points
 * will not send insert / uninserted messages
 */
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

}  // namespace

//
// Delete Wires
//

auto delete_temporary_wire_segment(CircuitData& circuit, segment_part_t& segment_part)
    -> void {
    if (!segment_part) [[unlikely]] {
        throw std::runtime_error("segment part is invalid");
    }
    if (!is_temporary(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only delete temporary segments");
    }

    remove_segment_from_tree(circuit, segment_part);
}

//
// Move Wires
//

auto is_wire_position_representable(const Layout& layout,
                                    const segment_part_t segment_part, int dx, int dy)
    -> bool {
    if (!segment_part) [[unlikely]] {
        throw std::runtime_error("segment part is invalid");
    }

    const auto line = get_line(layout, segment_part);
    return is_representable(line, dx, dy);
}

auto move_temporary_wire_unchecked(Layout& layout, segment_t segment,
                                   part_t verify_full_part, int dx, int dy) -> void {
    assert(is_temporary(segment.wire_id));
    assert(verify_full_part == to_part(get_line(layout, segment)));
    assert(is_wire_position_representable(
        layout, segment_part_t {segment, verify_full_part}, dx, dy));

    auto& m_tree = layout.wires().modifiable_segment_tree(segment.wire_id);

    auto info = m_tree.info(segment.segment_index);
    info.line = add_unchecked(info.line, dx, dy);

    if (to_part(info.line) != verify_full_part) {
        throw std::runtime_error("need to select full line part");
    }

    m_tree.update_segment(segment.segment_index, info);
}

auto move_or_delete_temporary_wire(CircuitData& circuit, segment_part_t& segment_part,
                                   int dx, int dy) -> void {
    if (!segment_part) [[unlikely]] {
        throw std::runtime_error("segment part is invalid");
    }
    if (!is_temporary(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only move temporary segments");
    }

    if (!is_wire_position_representable(circuit.layout, segment_part, dx, dy)) {
        // delete
        remove_segment_from_tree(circuit, segment_part);
        return;
    }

    const auto full_line = get_line(circuit.layout, segment_part.segment);
    const auto part_line = to_line(full_line, segment_part.part);

    if (full_line != part_line) {
        move_segment_between_trees(circuit, segment_part, segment_part.segment.wire_id);
    }

    // move
    auto& m_tree =
        circuit.layout.wires().modifiable_segment_tree(segment_part.segment.wire_id);
    auto info = m_tree.info(segment_part.segment.segment_index);
    info.line = add_unchecked(part_line, dx, dy);
    m_tree.update_segment(segment_part.segment.segment_index, info);

    // TODO bug missing moved messages for selection updates,
    //      maybe use a pre-build method for this?

    // messages
    if (full_line == part_line) {  // otherwise already sent in move_segment above
        circuit.submit(info_message::SegmentCreated {segment_part.segment});
    }
}

//
// Change Insertion Mode
//

namespace {

auto is_wire_with_segments(const Layout& layout, const wire_id_t wire_id) -> bool {
    return !layout.wires().segment_tree(wire_id).empty();
}

auto notify_wire_id_change(CircuitData& circuit, const wire_id_t new_wire_id,
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

auto swap_and_delete_empty_wire(CircuitData& circuit, wire_id_t& wire_id,
                                wire_id_t* preserve_element = nullptr) -> void {
    if (!wire_id) [[unlikely]] {
        throw std::runtime_error("element id is invalid");
    }

    if (!is_inserted(wire_id)) [[unlikely]] {
        std::runtime_error("can only delete inserted wires");
    }
    if (is_wire_with_segments(circuit.layout, wire_id)) [[unlikely]] {
        std::runtime_error("can't delete wires with segments");
    }

    // delete in underlying
    auto last_id = circuit.layout.wires().swap_and_delete(wire_id);

    if (wire_id != last_id) {
        notify_wire_id_change(circuit, wire_id, last_id);
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

auto add_new_wire_element(Layout& layout) -> wire_id_t {
    return layout.wires().add_wire();
}

auto reset_segment_endpoints(Layout& layout, const segment_t segment) {
    if (is_inserted(segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("cannot reset endpoints of inserted wire segment");
    }
    auto& m_tree = layout.wires().modifiable_segment_tree(segment.wire_id);

    const auto new_info = segment_info_t {
        .line = m_tree.line(segment.segment_index),
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };

    m_tree.update_segment(segment.segment_index, new_info);
}

auto wire_endpoints_colliding(const CircuitData& circuit, ordered_line_t line) -> bool {
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

auto is_wire_colliding(const CircuitData& circuit, const ordered_line_t line) -> bool {
    return wire_endpoints_colliding(circuit, line) ||
           circuit.index.collision_index().is_colliding(line);
}

auto get_display_states(const Layout& layout, const segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t> {
    using enum display_state_t;

    const auto& tree = layout.wires().segment_tree(segment_part.segment.wire_id);
    const auto tree_state = to_display_state(segment_part.segment.wire_id);

    // aggregates
    if (tree_state == temporary || tree_state == colliding) {
        return std::make_pair(tree_state, tree_state);
    }

    // check valid parts
    for (const auto valid_part : tree.valid_parts(segment_part.segment.segment_index)) {
        // parts can not touch or overlap, so we can return early
        if (a_inside_b(segment_part.part, valid_part)) {
            return std::make_pair(valid, valid);
        }
        if (a_overlaps_any_of_b(segment_part.part, valid_part)) {
            return std::make_pair(valid, normal);
        }
    }
    return std::make_pair(normal, normal);
}

auto get_insertion_modes(const Layout& layout, const segment_part_t segment_part)
    -> std::pair<InsertionMode, InsertionMode> {
    const auto display_states = get_display_states(layout, segment_part);

    return std::make_pair(to_insertion_mode(display_states.first),
                          to_insertion_mode(display_states.second));
}

auto updated_segment_info(segment_info_t segment_info, const point_t position,
                          const SegmentPointType point_type) -> segment_info_t {
    if (segment_info.line.p0 == position) {
        segment_info.p0_type = point_type;
    } else if (segment_info.line.p1 == position) {
        segment_info.p1_type = point_type;
    } else {
        throw std::runtime_error("Position needs to be an endpoint of the segment.");
    }
    return segment_info;
}

using point_update_t =
    std::initializer_list<const std::pair<segment_index_t, SegmentPointType>>;

auto update_segment_point_types(CircuitData& circuit, wire_id_t wire_id,
                                point_update_t data, const point_t position) -> void {
    if (data.size() == 0) {
        return;
    }
    if (!is_inserted(wire_id)) [[unlikely]] {
        throw std::runtime_error("only works for inserted segment trees.");
    }
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);

    const auto run_point_update = [&](bool set_to_shadow) {
        for (auto [segment_index, point_type] : data) {
            const auto old_info = m_tree.info(segment_index);
            const auto new_info = updated_segment_info(
                old_info, position,
                set_to_shadow ? SegmentPointType::shadow_point : point_type);

            if (old_info != new_info) {
                m_tree.update_segment(segment_index, new_info);

                circuit.submit(info_message::InsertedEndPointsUpdated {
                    .segment = segment_t {wire_id, segment_index},
                    .new_segment_info = new_info,
                    .old_segment_info = old_info,
                });
            }
        }
    };

    // first empty caches
    run_point_update(true);
    // write the new states
    run_point_update(false);
}

auto sort_through_lines_first(std::span<std::pair<ordered_line_t, segment_index_t>> lines,
                              const point_t point) -> void {
    std::ranges::sort(lines, {},
                      [point](std::pair<ordered_line_t, segment_index_t> item) {
                          return is_endpoint(point, item.first);
                      });
}

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
        circuit.submit(info_message::SegmentInserted {segment_0, info_merged});
    }

    if (to_part(info_0.line) != to_part(info_merged.line, info_0.line)) {
        circuit.submit(info_message::SegmentPartMoved {
            .segment_part_destination =
                segment_part_t {segment_0, to_part(info_merged.line, info_0.line)},
            .segment_part_source = segment_part_t {segment_0, to_part(info_0.line)},
        });
    }

    circuit.submit(info_message::SegmentPartMoved {
        .segment_part_destination =
            segment_part_t {segment_0, to_part(info_merged.line, info_1.line)},
        .segment_part_source = segment_part_t {segment_1, to_part(info_1.line)},
    });

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

auto merge_line_segments(CircuitData& circuit, segment_t segment_0, segment_t segment_1,
                         segment_part_t* preserve_segment) -> void {
    if (segment_0.segment_index < segment_1.segment_index) {
        _merge_line_segments_ordered(circuit, segment_0, segment_1, preserve_segment);
    } else {
        _merge_line_segments_ordered(circuit, segment_1, segment_0, preserve_segment);
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

auto fix_and_merge_segments(CircuitData& circuit, const point_t position,
                            segment_part_t* preserve_segment = nullptr) -> void {
    const auto segments = circuit.index.selection_index().query_line_segments(position);
    const auto segment_count = get_segment_count(segments);

    if (segment_count == 0) [[unlikely]] {
        return;
        // throw_exception("Could not find any segments at position.");
    }
    const auto wire_id = get_unique_wire_id(segments);
    const auto indices = get_segment_indices(segments);

    if (segment_count == 1) {
        const auto new_type = get_segment_point_type(circuit.layout, segments.at(0),
                                                     position) == SegmentPointType::input
                                  ? SegmentPointType::input
                                  : SegmentPointType::output;

        update_segment_point_types(circuit, wire_id,
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
        sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            split_line_segment(circuit, segment_t {wire_id, lines.at(0).second},
                               position);
            fix_and_merge_segments(circuit, position, preserve_segment);
            return;
        }

        const auto horizontal_0 = is_horizontal(lines.at(0).first);
        const auto horizontal_1 = is_horizontal(lines.at(1).first);
        const auto parallel = horizontal_0 == horizontal_1;

        if (parallel) {
            merge_line_segments(circuit, segments.at(0), segments.at(1),
                                preserve_segment);
            return;
        }

        // this handles corners
        update_segment_point_types(
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
        sort_through_lines_first(lines, position);
        const auto has_through_line_0 = !is_endpoint(position, lines.at(0).first);

        if (has_through_line_0) {
            throw std::runtime_error("This is not allowed, segment be split");
        } else {
            update_segment_point_types(
                circuit, wire_id,
                {
                    std::pair {indices.at(0), SegmentPointType::cross_point},
                    std::pair {indices.at(1), SegmentPointType::shadow_point},
                    std::pair {indices.at(2), SegmentPointType::shadow_point},
                },
                position);
        }
        return;
    }

    if (segment_count == 4) {
        update_segment_point_types(
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

    throw std::runtime_error("unexpected unhandeled case");
}

// we assume we get a valid tree where the part between p0 and p1
// has been removed this method puts the segments at p1 into a new tree
auto split_broken_tree(CircuitData& circuit, point_t p0, point_t p1) -> wire_id_t {
    const auto p0_tree_id = circuit.index.collision_index().get_first_wire(p0);
    const auto p1_tree_id = circuit.index.collision_index().get_first_wire(p1);

    if (!p0_tree_id || !p1_tree_id || p0_tree_id != p1_tree_id) {
        return null_wire_id;
    };

    // create new tree
    const auto new_tree_id = add_new_wire_element(circuit.layout);

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

    assert(is_contiguous_tree(tree_from));
    assert(is_contiguous_tree(circuit.layout.wires().segment_tree(new_tree_id)));

    return new_tree_id;
}

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

auto find_wire_for_inserting_segment(CircuitData& circuit,
                                     const segment_part_t segment_part) -> wire_id_t {
    const auto line = get_line(circuit.layout, segment_part);

    auto candidate_0 = circuit.index.collision_index().get_first_wire(line.p0);
    auto candidate_1 = circuit.index.collision_index().get_first_wire(line.p1);

    // 1 wire
    if (bool {candidate_0} ^ bool {candidate_1}) {
        return candidate_0 ? candidate_0 : candidate_1;
    }

    // 2 wires
    if (candidate_0 && candidate_1) {
        // we assume segment is part of aggregates that have ID 0 and 1
        if (segment_part.segment.wire_id > candidate_0 ||
            segment_part.segment.wire_id > candidate_1) {
            throw std::runtime_error("cannot preserve segment wire_id");
        }

        if (candidate_0 > candidate_1) {
            using std::swap;
            swap(candidate_0, candidate_1);
        }

        merge_and_delete_tree(circuit, candidate_0, candidate_1);
        return candidate_0;
    }

    // 0 wires
    return add_new_wire_element(circuit.layout);
}

auto discover_wire_inputs(CircuitData& circuit, segment_t segment) {
    const auto line = get_line(circuit.layout, segment);

    // find LogicItem outputs
    if (const auto entry = circuit.index.logicitem_output_index().find(line.p0)) {
        auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
        auto info = m_tree.info(segment.segment_index);

        info.p0_type = SegmentPointType::input;
        m_tree.update_segment(segment.segment_index, info);
    }
    if (const auto entry = circuit.index.logicitem_output_index().find(line.p1)) {
        auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
        auto info = m_tree.info(segment.segment_index);

        info.p1_type = SegmentPointType::input;
        m_tree.update_segment(segment.segment_index, info);
    }
}

auto insert_wire(CircuitData& circuit, segment_part_t& segment_part) -> void {
    if (is_inserted(segment_part.segment.wire_id)) {
        throw std::runtime_error("segment is already inserted");
    }
    const auto target_wire_id = find_wire_for_inserting_segment(circuit, segment_part);

    reset_segment_endpoints(circuit.layout, segment_part.segment);
    discover_wire_inputs(circuit, segment_part.segment);
    move_segment_between_trees(circuit, segment_part, target_wire_id);

    const auto line = get_line(circuit.layout, segment_part);
    fix_and_merge_segments(circuit, line.p0, &segment_part);
    fix_and_merge_segments(circuit, line.p1, &segment_part);

    assert(is_contiguous_tree(circuit.layout.wires().segment_tree(target_wire_id)));
}

auto mark_valid(Layout& layout, const segment_part_t segment_part) {
    auto& m_tree = layout.wires().modifiable_segment_tree(segment_part.segment.wire_id);
    m_tree.mark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto unmark_valid(Layout& layout, const segment_part_t segment_part) {
    auto& m_tree = layout.wires().modifiable_segment_tree(segment_part.segment.wire_id);
    m_tree.unmark_valid(segment_part.segment.segment_index, segment_part.part);
}

auto _wire_change_temporary_to_colliding(CircuitData& circuit,
                                         segment_part_t& segment_part) -> void {
    const auto line = get_line(circuit.layout, segment_part);
    bool colliding = is_wire_colliding(circuit, line);

    if (colliding) {
        const auto destination = colliding_wire_id;
        move_segment_between_trees(circuit, segment_part, destination);
        reset_segment_endpoints(circuit.layout, segment_part.segment);
    } else {
        insert_wire(circuit, segment_part);
        mark_valid(circuit.layout, segment_part);
    }
}

auto _wire_change_colliding_to_insert(CircuitData& circuit, segment_part_t& segment_part)
    -> void {
    const auto wire_id = segment_part.segment.wire_id;

    // from valid
    if (is_inserted(wire_id)) {
        unmark_valid(circuit.layout, segment_part);
    }

    // from colliding
    else if (is_colliding(wire_id)) {
        remove_segment_from_tree(circuit, segment_part);
    }

    else {
        throw std::runtime_error("wire needs to be in inserted or colliding state");
    }
}

auto _wire_change_insert_to_colliding(Layout& layout, segment_part_t& segment_part)
    -> void {
    mark_valid(layout, segment_part);
}

auto _wire_change_colliding_to_temporary(CircuitData& circuit,
                                         segment_part_t& segment_part) -> void {
    auto source_id = segment_part.segment.wire_id;
    const auto was_inserted = is_inserted(segment_part.segment.wire_id);
    const auto moved_line = get_line(circuit.layout, segment_part);

    if (was_inserted) {
        unmark_valid(circuit.layout, segment_part);
    }

    // move to temporary
    const auto destination_id = temporary_wire_id;
    move_segment_between_trees(circuit, segment_part, destination_id);

    if (was_inserted) {
        if (circuit.layout.wires().segment_tree(source_id).empty()) {
            swap_and_delete_empty_wire(circuit, source_id, &segment_part.segment.wire_id);
        } else {
            fix_and_merge_segments(circuit, moved_line.p0);
            fix_and_merge_segments(circuit, moved_line.p1);

            split_broken_tree(circuit, moved_line.p0, moved_line.p1);
        }
        reset_segment_endpoints(circuit.layout, segment_part.segment);
    }
}

}  // namespace

auto change_wire_insertion_mode(CircuitData& circuit, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void {
    if (!segment_part) [[unlikely]] {
        throw std::runtime_error("segment part is invalid");
    }

    // as parts have length, the line segment can have two possible modes
    // a part could be in state valid (insert_or_discard) and another in state normal
    const auto old_modes = get_insertion_modes(circuit.layout, segment_part);

    if (old_modes.first == new_mode && old_modes.second == new_mode) {
        return;
    }

    if (old_modes.first == InsertionMode::temporary ||
        old_modes.second == InsertionMode::temporary) {
        _wire_change_temporary_to_colliding(circuit, segment_part);
    }
    if (new_mode == InsertionMode::insert_or_discard) {
        _wire_change_colliding_to_insert(circuit, segment_part);
    }
    if (old_modes.first == InsertionMode::insert_or_discard ||
        old_modes.second == InsertionMode::insert_or_discard) {
        _wire_change_insert_to_colliding(circuit.layout, segment_part);
    }
    if (new_mode == InsertionMode::temporary) {
        _wire_change_colliding_to_temporary(circuit, segment_part);
    }
}

//
// Add Wire
//

namespace {

auto add_segment_to_tree(CircuitData& circuit, const wire_id_t wire_id,
                         ordered_line_t line) -> segment_part_t {
    // insert new segment
    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(wire_id);

    const auto segment_info = segment_info_t {
        .line = line,
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    };
    const auto segment_index = m_tree.add_segment(segment_info);
    const auto segment = segment_t {wire_id, segment_index};

    // messages
    circuit.submit(info_message::SegmentCreated {segment});
    if (is_inserted(wire_id)) {
        circuit.submit(info_message::SegmentInserted {segment, segment_info});
    }

    return segment_part_t {segment, to_part(line)};
}

}  // namespace

auto add_wire_segment(CircuitData& circuit, ordered_line_t line,
                      InsertionMode insertion_mode) -> segment_part_t {
    auto segment_part = add_segment_to_tree(circuit, temporary_wire_id, line);

    // auto segment_part = add_segment_to_aggregate(state.layout, state.sender, line,
    //                                              display_state_t::temporary);

    change_wire_insertion_mode(circuit, segment_part, insertion_mode);

    return segment_part;
}

//
// Toggle Crosspoint
//

namespace {

auto delete_all_inserted_wires(CircuitData& circuit, point_t point) -> void {
    // segment ids change during deletion, so we need to query after each deletion
    while (true) {
        const auto segments = circuit.index.selection_index().query_line_segments(point);

        if (!segments.at(0)) {
            return;
        }
        if (!is_inserted(segments.at(0).wire_id)) [[unlikely]] {
            throw std::runtime_error("only works on inserted elements");
        }

        const auto line = get_line(circuit.layout, segments.at(0));
        auto segment_part = segment_part_t {segments.at(0), to_part(line)};

        change_wire_insertion_mode(circuit, segment_part, InsertionMode::temporary);
        delete_temporary_wire_segment(circuit, segment_part);
    }
}

auto remove_wire_crosspoint(CircuitData& circuit, point_t point) -> void {
    const auto segments = circuit.index.selection_index().query_line_segments(point);
    const auto segment_count = get_segment_count(segments);

    if (segment_count != 4) {
        return;
    }
    if (!all_same_wire_id(segments)) [[unlikely]] {
        throw std::runtime_error("expected query result to of one segment tree");
    }

    auto lines = std::array {
        get_line(circuit.layout, segments.at(0)),
        get_line(circuit.layout, segments.at(1)),
        get_line(circuit.layout, segments.at(2)),
        get_line(circuit.layout, segments.at(3)),
    };
    std::ranges::sort(lines);
    const auto new_line_0 = ordered_line_t {lines.at(0).p0, lines.at(3).p1};
    const auto new_line_1 = ordered_line_t {lines.at(1).p0, lines.at(2).p1};

    delete_all_inserted_wires(circuit, point);
    add_wire_segment(circuit, new_line_0, InsertionMode::insert_or_discard);
    add_wire_segment(circuit, new_line_1, InsertionMode::insert_or_discard);
}

auto add_wire_crosspoint(CircuitData& circuit, point_t point) -> void {
    const auto segments = circuit.index.selection_index().query_line_segments(point);
    const auto segment_count = get_segment_count(segments);

    if (segment_count != 2) {
        return;
    }

    const auto wire_id_0 = segments.at(0).wire_id;
    const auto wire_id_1 = segments.at(1).wire_id;

    if (wire_id_0 == wire_id_1) {
        return;
    }
    if (circuit.layout.wires().segment_tree(wire_id_0).input_count() +
            circuit.layout.wires().segment_tree(wire_id_1).input_count() >
        connection_count_t {1}) {
        return;
    }

    if (!is_inserted(wire_id_0) || !is_inserted(wire_id_1)) [[unlikely]] {
        throw std::runtime_error("only works on inserted elements");
    }

    const auto line0 = get_line(circuit.layout, segments.at(0));
    const auto line1 = get_line(circuit.layout, segments.at(1));

    delete_all_inserted_wires(circuit, point);

    const auto mode = InsertionMode::insert_or_discard;
    add_wire_segment(circuit, ordered_line_t {line0.p0, point}, mode);
    add_wire_segment(circuit, ordered_line_t {point, line0.p1}, mode);
    add_wire_segment(circuit, ordered_line_t {line1.p0, point}, mode);
    add_wire_segment(circuit, ordered_line_t {point, line1.p1}, mode);
}

}  // namespace

auto toggle_inserted_wire_crosspoint(CircuitData& circuit, point_t point) -> void {
    if (circuit.index.collision_index().is_wires_crossing(point)) {
        add_wire_crosspoint(circuit, point);
    }

    else if (circuit.index.collision_index().is_wire_cross_point(point)) {
        remove_wire_crosspoint(circuit, point);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
