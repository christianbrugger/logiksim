#include "core/component/editable_circuit/editing/edit_wire.h"

#include "core/algorithm/make_unique.h"
#include "core/algorithm/transform_to_vector.h"
#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/editing/edit_wire_detail.h"
#include "core/geometry/line.h"
#include "core/geometry/offset.h"
#include "core/geometry/orientation.h"
#include "core/geometry/segment_info.h"
#include "core/index/segment_map.h"
#include "core/index/spatial_point_index.h"
#include "core/layout.h"
#include "core/tree_normalization.h"
#include "core/vocabulary/endpoints.h"

#include <algorithm>

namespace logicsim {

namespace editable_circuit {

namespace editing {

namespace {

auto _store_history_segment_create_temporary(CircuitData& circuit,
                                             segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        assert(is_full_segment(circuit.layout, segment_part));
        const auto segment_key = circuit.index.key_index().get(segment_part.segment);

        // if (circuit.visible_selection.initial_selection().is_selected(
        //         segment_part.segment)) {
        //     stack->push_segment_add_visible_selection();
        // }

        const auto info = get_segment_info(circuit.layout, segment_part.segment);

        if (info.p0_type != SegmentPointType::shadow_point ||
            info.p1_type != SegmentPointType::shadow_point) {
            stack->push_segment_set_endpoints(segment_key, get_endpoints(info));
        }
        stack->push_segment_create_temporary(segment_key, info.line);
    }
}

auto move_segment_between_trees_with_history(
    CircuitData& circuit, segment_part_t& segment_part,
    const wire_id_t destination_id) -> std::pair<segment_part_t, segment_part_t> {
    auto stack = circuit.history.get_stack();

    const auto [left1, left2] =
        move_segment_between_trees(circuit, segment_part, destination_id);

    assert(is_full_segment(circuit.layout, segment_part));
    assert(!left1 || is_full_segment(circuit.layout, left1));
    assert(!left2 || is_full_segment(circuit.layout, left2));

    if (stack != nullptr && left1 && !left2) {
        const auto key_0 = circuit.index.key_index().get(segment_part.segment);
        const auto key_1 = circuit.index.key_index().get(left1.segment);

        stack->push_segment_merge(key_0, key_1);
    }

    if (stack != nullptr && left1 && left2) {
        // key_0 is between key_1 and key_2 => key_0 can be merged with both
        const auto key_0 = circuit.index.key_index().get(segment_part.segment);
        const auto key_1 = circuit.index.key_index().get(left1.segment);
        const auto key_2 = circuit.index.key_index().get(left2.segment);

        // restored in reverse order
        stack->push_segment_merge(key_0, key_1);
        stack->push_segment_merge(key_0, key_2);
    }

    return {left1, left2};
}

auto _store_history_segment_move_temporary(CircuitData& circuit, segment_t segment,
                                           move_delta_t delta) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto segment_key = circuit.index.key_index().get(segment);
        stack->push_segment_move_temporary(segment_key, delta);
    }
}

auto _store_history_segment_colliding_to_temporary(CircuitData& circuit,
                                                   segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto segment_key = circuit.index.key_index().get(segment_part.segment);
        stack->push_segment_colliding_to_temporary(segment_key, segment_part.part);
    }
}

auto _store_history_segment_temporary_to_colliding(CircuitData& circuit,
                                                   segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto segment_key = circuit.index.key_index().get(segment_part.segment);
        stack->push_segment_temporary_to_colliding(segment_key, segment_part.part);
    }
}

auto _store_history_segment_colliding_to_insert(CircuitData& circuit,
                                                segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto segment_key = circuit.index.key_index().get(segment_part.segment);
        stack->push_segment_colliding_to_insert(segment_key, segment_part.part);
    }
}

auto _store_history_segment_insert_to_colliding(CircuitData& circuit,
                                                segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto segment_key = circuit.index.key_index().get(segment_part.segment);
        stack->push_segment_insert_to_colliding(segment_key, segment_part.part);
    }
}

auto _store_history_segment_set_endpoints(CircuitData& circuit, segment_t segment,
                                          endpoints_t new_endpoints) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto endpoints = get_endpoints(get_segment_info(circuit.layout, segment));

        if (new_endpoints != endpoints) {
            const auto segment_key = circuit.index.key_index().get(segment);
            stack->push_segment_set_endpoints(segment_key, endpoints);
        }
    }
}

auto _store_history_segment_delete_temporary(CircuitData& circuit,
                                             segment_part_t segment_part) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        assert(is_full_segment(circuit.layout, segment_part));

        const auto segment_key = circuit.index.key_index().get(segment_part.segment);
        stack->push_segment_delete_temporary(segment_key);
    }
}

}  // namespace

//
// Delete Wires
//

auto delete_temporary_wire_segment(CircuitData& circuit,
                                   segment_part_t& segment_part) -> void {
    if (!is_temporary(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only delete temporary segments");
    }

    move_segment_between_trees_with_history(circuit, segment_part, temporary_wire_id);

    _store_history_segment_create_temporary(circuit, segment_part);
    remove_full_segment_from_uninserted_tree(circuit, segment_part);
}

//
// Move Wires
//

auto is_wire_position_representable(const Layout& layout,
                                    const segment_part_t segment_part,
                                    move_delta_t delta) -> bool {
    const auto line = get_line(layout, segment_part);
    return is_representable(line, delta.x, delta.y);
}

auto new_wire_positions_representable(const Layout& layout, const Selection& selection,
                                      move_delta_t delta) -> bool {
    const auto segment_representable = [&, delta](const selection::segment_pair_t& pair) {
        const auto& [segment, parts] = pair;
        const auto part_representable =
            [delta, full_line = get_line(layout, segment)](const part_t& part) {
                return is_representable(to_line(full_line, part), delta.x, delta.y);
            };

        return std::ranges::all_of(parts, part_representable);
    };

    return std::ranges::all_of(selection.selected_segments(), segment_representable);
}

auto move_temporary_wire_unchecked(CircuitData& circuit, segment_t segment,
                                   move_delta_t delta) -> void {
    assert(is_temporary(segment.wire_id));
    assert(is_wire_position_representable(
        circuit.layout, get_segment_part(circuit.layout, segment), delta));

    auto& m_tree = circuit.layout.wires().modifiable_segment_tree(segment.wire_id);
    auto info = m_tree.info(segment.segment_index);
    info.line = add_unchecked(info.line, delta.x, delta.y);
    m_tree.update_segment(segment.segment_index, info);

    _store_history_segment_move_temporary(circuit, segment, -delta);
}

auto move_temporary_wire_unchecked(CircuitData& circuit, segment_part_t full_segment_part,
                                   move_delta_t delta) -> void {
    assert(is_full_segment(circuit.layout, full_segment_part));

    move_temporary_wire_unchecked(circuit, full_segment_part.segment, delta);
}

auto move_or_delete_temporary_wire(CircuitData& circuit, segment_part_t& segment_part,
                                   move_delta_t delta) -> void {
    if (!is_temporary(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only move temporary segments");
    }

    if (!is_wire_position_representable(circuit.layout, segment_part, delta)) {
        delete_temporary_wire_segment(circuit, segment_part);
        return;
    }

    move_segment_between_trees_with_history(circuit, segment_part,
                                            segment_part.segment.wire_id);
    move_temporary_wire_unchecked(circuit, segment_part, delta);
}

//
// Change Insertion Mode
//

namespace {

auto _find_wire_for_inserting_segment(CircuitData& circuit,
                                      const segment_part_t segment_part) -> wire_id_t {
    if (is_inserted(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only find wire for uninserted segments");
    }

    const auto line = get_line(circuit.layout, segment_part);

    auto candidate_0 = circuit.index.collision_index().get_first_wire(line.p0);
    auto candidate_1 = circuit.index.collision_index().get_first_wire(line.p1);

    // 1 wire
    if ((bool {candidate_0}) ^ (bool {candidate_1})) {
        return candidate_0 ? candidate_0 : candidate_1;
    }

    // 2 wires
    if (candidate_0 && candidate_1) {
        // This is needed, so segment_part.segment.wire_id is preserved
        // It should always be true, as uinserted wires have wire_id 0 and 1
        Expects(segment_part.segment.wire_id < candidate_0);
        Expects(segment_part.segment.wire_id < candidate_1);

        if (candidate_0 > candidate_1) {
            using std::swap;
            swap(candidate_0, candidate_1);
        }

        merge_and_delete_tree(circuit, candidate_0, candidate_1);
        return candidate_0;
    }

    // 0 wires
    return circuit.layout.wires().add_wire();
}

auto _insert_uninserted_segment(CircuitData& circuit,
                                segment_part_t& segment_part) -> void {
    if (is_inserted(segment_part.segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("segment is already inserted");
    }
    const auto target_wire_id = _find_wire_for_inserting_segment(circuit, segment_part);

    reset_segment_endpoints_with_history(circuit, segment_part.segment);
    set_wire_inputs_at_logicitem_outputs(circuit, segment_part.segment);
    move_segment_between_trees_with_history(circuit, segment_part, target_wire_id);

    const auto line = get_line(circuit.layout, segment_part);
    fix_and_merge_segments(circuit, line.p0, &segment_part);
    fix_and_merge_segments(circuit, line.p1, &segment_part);

    assert(is_contiguous_tree_with_correct_endpoints(
        std::as_const(circuit).layout.wires().segment_tree(target_wire_id)));
}

auto _wire_change_temporary_to_colliding(CircuitData& circuit,
                                         segment_part_t& segment_part) -> void {
    const auto line = get_line(circuit.layout, segment_part);
    bool colliding = is_wire_colliding(circuit, line);

    if (colliding) {
        const auto destination = colliding_wire_id;
        move_segment_between_trees_with_history(circuit, segment_part, destination);
        reset_segment_endpoints_with_history(circuit, segment_part.segment);
    } else {
        _insert_uninserted_segment(circuit, segment_part);
        mark_valid(circuit.layout, segment_part);
    }

    _store_history_segment_colliding_to_temporary(circuit, segment_part);
}

auto _wire_change_insert_to_colliding(CircuitData& circuit,
                                      const segment_part_t segment_part) -> void {
    mark_valid(circuit.layout, segment_part);

    _store_history_segment_colliding_to_insert(circuit, segment_part);
}

auto _wire_change_colliding_to_temporary(CircuitData& circuit,
                                         segment_part_t& segment_part) -> void {
    auto source_id = segment_part.segment.wire_id;
    const auto was_inserted = is_inserted(segment_part.segment.wire_id);

    if (was_inserted) {
        unmark_valid(circuit.layout, segment_part);
    }

    // move to temporary
    const auto destination_id = temporary_wire_id;

    if (was_inserted) {
        move_segment_between_trees(circuit, segment_part, destination_id);
    } else {
        move_segment_between_trees_with_history(circuit, segment_part, destination_id);
    }

    if (was_inserted) {
        if (circuit.layout.wires().segment_tree(source_id).empty()) {
            swap_and_delete_empty_wire(circuit, source_id, &segment_part.segment.wire_id);
        } else {
            const auto moved_line = get_line(circuit.layout, segment_part);
            fix_and_merge_segments(circuit, moved_line.p0);
            fix_and_merge_segments(circuit, moved_line.p1);

            split_broken_tree(circuit, moved_line.p0, moved_line.p1);
        }
        reset_uninserted_endpoints(circuit.layout, segment_part.segment);
    }

    _store_history_segment_temporary_to_colliding(circuit, segment_part);
}

auto _wire_change_colliding_to_insert(CircuitData& circuit,
                                      segment_part_t& segment_part) -> void {
    const auto wire_id = segment_part.segment.wire_id;

    // from valid
    if (is_inserted(wire_id)) {
        unmark_valid(circuit.layout, segment_part);
        _store_history_segment_insert_to_colliding(circuit, segment_part);
        return;
    }

    // from colliding
    if (is_colliding(wire_id)) {
        _wire_change_colliding_to_temporary(circuit, segment_part);
        delete_temporary_wire_segment(circuit, segment_part);
        return;
    }

    throw std::runtime_error("wire needs to be in inserted or colliding state");
}

}  // namespace

auto change_wire_insertion_mode(CircuitData& circuit, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void {
    // As segments have length, the given segment can have two possible modes.
    // The mixed state could be:
    //   + InsertionMode::collisions (display_state_t::valid)
    //   + InsertionMode::insert_or_discard (display_state_t::normal)
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
        _wire_change_insert_to_colliding(circuit, segment_part);
    }
    if (new_mode == InsertionMode::temporary) {
        _wire_change_colliding_to_temporary(circuit, segment_part);
    }
}

//
// Add Wire
//

auto add_wire_segment(CircuitData& circuit, ordered_line_t line,
                      InsertionMode insertion_mode,
                      segment_key_t segment_key) -> segment_part_t {
    auto segment_part = add_temporary_segment(circuit, line);

    if (segment_key) {
        circuit.index.set_key(segment_part.segment, segment_key);
    }
    _store_history_segment_delete_temporary(circuit, segment_part);

    change_wire_insertion_mode(circuit, segment_part, insertion_mode);

    return segment_part;
}

//
// Toggle Crosspoint
//

namespace {

auto _delete_all_selectable_wires_at(CircuitData& circuit, point_t point) -> void {
    // segment ids change during deletion, new query is needed after each deletion
    while (true) {
        const auto segments = circuit.index.selection_index().query_line_segments(point);

        if (!segments.at(0)) {
            return;
        }
        if (!is_inserted(segments.at(0).wire_id)) [[unlikely]] {
            throw std::runtime_error("only works on inserted elements");
        }

        auto segment_part = get_segment_part(circuit.layout, segments.at(0));

        change_wire_insertion_mode(circuit, segment_part, InsertionMode::temporary);
        delete_temporary_wire_segment(circuit, segment_part);
    }
}

auto _remove_wire_crosspoint(CircuitData& circuit, point_t point) -> void {
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

    _delete_all_selectable_wires_at(circuit, point);
    add_wire_segment(circuit, new_line_0, InsertionMode::insert_or_discard);
    add_wire_segment(circuit, new_line_1, InsertionMode::insert_or_discard);
}

auto _add_wire_crosspoint(CircuitData& circuit, point_t point) -> void {
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

    _delete_all_selectable_wires_at(circuit, point);

    const auto mode = InsertionMode::insert_or_discard;
    add_wire_segment(circuit, ordered_line_t {line0.p0, point}, mode);
    add_wire_segment(circuit, ordered_line_t {point, line0.p1}, mode);
    add_wire_segment(circuit, ordered_line_t {line1.p0, point}, mode);
    add_wire_segment(circuit, ordered_line_t {point, line1.p1}, mode);
}

}  // namespace

auto toggle_wire_crosspoint(CircuitData& circuit, point_t point) -> void {
    if (circuit.index.collision_index().is_wires_crossing(point)) {
        _add_wire_crosspoint(circuit, point);
    }

    else if (circuit.index.collision_index().is_wire_cross_point(point)) {
        _remove_wire_crosspoint(circuit, point);
    }
}

//
// Regularization
//

auto set_uninserted_endpoints_with_history(CircuitData& circuit, const segment_t segment,
                                           endpoints_t endpoints) -> void {
    _store_history_segment_set_endpoints(circuit, segment, endpoints);

    set_uninserted_endpoints(circuit.layout, segment, endpoints);
}

auto reset_segment_endpoints_with_history(CircuitData& circuit,
                                          const segment_t segment) -> void {
    set_uninserted_endpoints_with_history(
        circuit, segment,
        endpoints_t {.p0_type = SegmentPointType::shadow_point,
                     .p1_type = SegmentPointType::shadow_point});
}

auto set_uninserted_crosspoint_with_history(CircuitData& circuit, segment_t segment,
                                            point_t point) -> void {
    if (const auto stack = circuit.history.get_stack()) {
        const auto info = get_segment_info(circuit.layout, segment);

        if (get_segment_point_type(info, point) != SegmentPointType::cross_point) {
            const auto segment_key = circuit.index.key_index().get(segment);
            stack->push_segment_set_endpoints(segment_key, get_endpoints(info));
        }
    }

    set_uninserted_crosspoint(circuit.layout, segment, point);
}

struct merge_memory_t {
    segment_key_t key_0 {null_segment_key};
    segment_key_t key_1 {null_segment_key};
    ordered_line_t line_0 {};
    ordered_line_t line_1 {};
};

auto merge_uninserted_segment_with_history(CircuitData& circuit, segment_t segment_0,
                                           segment_t segment_1) -> segment_t {
    if (is_inserted(segment_0.wire_id) || is_inserted(segment_1.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only merge uninserted wires");
    }

    const auto stack = circuit.history.get_stack();

    // remember state
    const auto before = [&]() {
        if (stack != nullptr) {
            return merge_memory_t {
                .key_0 = circuit.index.key_index().get(segment_0),
                .key_1 = circuit.index.key_index().get(segment_1),
                .line_0 = get_line(circuit.layout, segment_0),
                .line_1 = get_line(circuit.layout, segment_1),
            };
        }
        return merge_memory_t {};
    }();

    // merge
    const auto segment_after =
        merge_line_segments(circuit, segment_0, segment_1, nullptr);

    // history
    if (stack != nullptr) {
        const auto key_after =
            before.line_0 < before.line_1 ? before.key_0 : before.key_1;
        assert(key_after == circuit.index.key_index().get(segment_after));

        const auto split_offset = to_offset(std::min(before.line_0, before.line_1));

        auto definition = split_segment_key_t {
            .source = key_after,
            .new_key = key_after == before.key_0 ? before.key_1 : before.key_0,
            .split_offset = split_offset,
        };

        stack->push_segment_split(definition);
    }

    return segment_after;
}

auto merge_all_line_segments_with_history(
    CircuitData& circuit, std::vector<std::pair<segment_t, segment_t>>& pairs) -> void {
    merge_all_line_segments(
        circuit, pairs,
        [](CircuitData& circuit_, segment_t segment_0, segment_t segment_1) {
            merge_uninserted_segment_with_history(circuit_, segment_0, segment_1);
        });
}

auto split_segment_t::format() const -> std::string {
    return fmt::format(
        "split_segment_t{{source_key = {}, new_key = {}, split_offset = {}}}", source_key,
        new_key, split_offset);
}

auto split_line_segment_with_history(CircuitData& circuit, const segment_t segment,
                                     offset_t offset) -> std::pair<segment_t, segment_t> {
    auto move_segment_part = segment_part_t {segment, part_t {0, offset}};

    const auto [left1, left2] = move_segment_between_trees_with_history(
        circuit, move_segment_part, segment.wire_id);

    Expects(left1);
    Expects(!left2);
    assert(is_full_segment(circuit.layout, left1));
    assert(is_full_segment(circuit.layout, move_segment_part));

    return {move_segment_part.segment, left1.segment};
}

auto split_line_segment_with_history(CircuitData& circuit, const segment_t segment,
                                     const point_t position) -> void {
    const auto full_line = get_line(circuit.layout, segment);
    const auto line_moved = ordered_line_t {position, full_line.p1};

    auto move_segment_part = segment_part_t {segment, to_part(full_line, line_moved)};
    move_segment_between_trees_with_history(circuit, move_segment_part, segment.wire_id);
}

auto split_uninserted_segment(CircuitData& circuit, split_segment_t definition) -> void {
    const auto source_segment = circuit.index.key_index().get(definition.source_key);

    if (is_inserted(source_segment.wire_id)) [[unlikely]] {
        throw std::runtime_error("can only split uninserted wires");
    }

    const auto [segment_0, segment_1] =
        split_line_segment_with_history(circuit, source_segment, definition.split_offset);

    // determine ordering
    const auto line_0 = get_line(circuit.layout, segment_0);
    const auto line_1 = get_line(circuit.layout, segment_1);
    const auto [first, second] = line_0 < line_1 ? std::pair {segment_0, segment_1}
                                                 : std::pair {segment_1, segment_0};

    // first set new key to avoid collision
    circuit.index.set_key(second, definition.new_key);
    circuit.index.set_key(first, definition.source_key);
}

auto regularize_temporary_selection(CircuitData& circuit, const Selection& selection,
                                    std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t> {
    if (true_cross_points) {
        split_temporary_segments(circuit, selection, *true_cross_points);
        std::ranges::sort(*true_cross_points);
    }

    const auto map = build_endpoint_map(circuit.layout, selection);
    auto mergeable_segments = adjacent_segments(map);
    auto cross_points = std::vector<point_t> {};

    iter_crosspoints(
        map, [&](point_t point, const segment_map::adjacent_segments_t& segments) {
            assert(segments.count() >= 3);
            using enum orientation_t;

            if (segments.count() == 3 || !true_cross_points ||
                std::ranges::binary_search(*true_cross_points, point)) {
                cross_points.push_back(point);

                const auto segment = segments.has(right)  //
                                         ? segments.at(right)
                                         : segments.at(left);
                set_uninserted_crosspoint_with_history(circuit, segment, point);
            } else {
                // merge wire crossings without true cross points
                mergeable_segments.emplace_back(segments.at(right), segments.at(left));
                mergeable_segments.emplace_back(segments.at(up), segments.at(down));
            }
        });

    merge_all_line_segments_with_history(circuit, mergeable_segments);

    return cross_points;
}

auto get_inserted_cross_points(const CircuitData& circuit,
                               const Selection& selection) -> std::vector<point_t> {
    auto cross_points = std::vector<point_t> {};

    for (const auto& [segment, parts] : selection.selected_segments()) {
        for (const auto& part : parts) {
            const auto line = get_line(circuit.layout, segment_part_t {segment, part});

            if (circuit.index.collision_index().is_wire_cross_point(line.p0)) {
                cross_points.push_back(line.p0);
            }
            if (circuit.index.collision_index().is_wire_cross_point(line.p1)) {
                cross_points.push_back(line.p1);
            }
        }
    }

    sort_and_make_unique(cross_points);
    return cross_points;
}

auto split_temporary_segments(CircuitData& circuit, const Selection& selection,
                              std::span<const point_t> split_points) -> void {
    const auto cache = SpatialPointIndex {split_points};

    const auto segments = transform_to_vector(
        selection.selected_segments(), [&](const Selection::segment_pair_t& value) {
            const auto& [segment, parts] = value;

            const auto full_line = get_line(circuit.layout, segment);

            if (!is_temporary(segment.wire_id)) {
                throw std::runtime_error("can only split temporary segments");
            }
            if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
                throw std::runtime_error(
                    "selection cannot contain partially selected lines");
            }

            return segment;
        });

    for (const auto& segment : segments) {
        const auto full_line = get_line(circuit.layout, segment);

        auto query_result = cache.query_intersects(full_line);
        sort_and_make_unique(query_result, std::greater<>());

        // splitting puts the second half into a new segment
        // so for this to work with multiple point, cross_points
        // need to be sorted in descendant order
        for (const auto& point : query_result) {
            if (is_inside(point, full_line)) {
                split_line_segment_with_history(circuit, segment, point);
            }
        }
    }
}

auto get_temporary_selection_splitpoints(
    const CircuitData& circuit, const Selection& selection) -> std::vector<point_t> {
    auto result = std::vector<point_t> {};

    const auto add_candidate = [&](point_t point) {
        const auto state = circuit.index.collision_index().query(point);
        if (state.is_wire_corner_point() || state.is_wire_connection() ||
            state.is_wire_cross_point()) {
            result.push_back(point);
        }
    };

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(circuit.layout, segment);

        if (!is_temporary(segment.wire_id)) {
            throw std::runtime_error(
                "can only find new split-points for temporary segments");
        }
        if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
            throw std::runtime_error("selection cannot contain partially selected lines");
        }

        if (is_horizontal(full_line)) {
            for (auto x : range(full_line.p0.x + grid_t {1}, full_line.p1.x)) {
                add_candidate(point_t {x, full_line.p0.y});
            }
        } else {
            for (auto y : range(full_line.p0.y + grid_t {1}, full_line.p1.y)) {
                add_candidate(point_t {full_line.p0.x, y});
            }
        }
    }

    return result;
}

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim
