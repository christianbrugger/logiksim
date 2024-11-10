#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_H

#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/point.h"

#include <optional>
#include <span>
#include <vector>

namespace logicsim {

struct part_t;
struct segment_t;
struct segment_part_t;
struct point_t;
struct move_delta_t;
struct ordered_line_t;
class Layout;
class Selection;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto delete_temporary_wire_segment(CircuitData& circuit,
                                   segment_part_t& segment_part) -> void;

[[nodiscard]] auto is_wire_position_representable(const Layout& layout,
                                                  segment_part_t segment_part,
                                                  move_delta_t delta) -> bool;

[[nodiscard]] auto new_wire_positions_representable(const Layout& layout,
                                                    const Selection& selection,
                                                    move_delta_t delta) -> bool;

// TODO why verify_full_part ??
auto move_temporary_wire_unchecked(Layout& layout, segment_t segment,
                                   part_t verify_full_part, move_delta_t delta) -> void;

auto move_or_delete_temporary_wire(CircuitData& circuit, segment_part_t& segment_part,
                                   move_delta_t delta) -> void;

auto change_wire_insertion_mode(CircuitData& circuit, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void;

auto add_wire_segment(CircuitData& circuit, ordered_line_t line,
                      InsertionMode insertion_mode) -> segment_part_t;

auto toggle_wire_crosspoint(CircuitData& circuit, point_t point) -> void;

//
// Regularization
//

/**
 * @brief: Regularizes temporary segments in the selection.
 *
 * Internally the functions first splits the segments at the given true cross points.
 * Then a map of all cross points (3 or 4 wires ending) are build and wires are further
 * split at those. Adjacent parallel wires without cross points are merged.
 *
 * Note, true_cross_points only need to be provided if the selection of segments
 * is partially inserted and uninserted multiple times. This is to resolve
 * the disambiguagion between two wires crossing with and without a cross-point.
 * True_cross_points are not required when a new group of wires is inserted initially
 * for example during copy and paste.
 *
 * Note, for new insertion of wires the returned list of crosspoints can be for
 * subsequent calls, to get to the same regularization.
 *
 * Throws, if any segment is not temporary.
 * Throws, if any segment is partially selected.
 *
 * Returns the internally build list of cross points.  */
auto regularize_temporary_selection(CircuitData& circuit, const Selection& selection,
                                    std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t>;

/**
 * @brief: Return a list of cross points (3 or 4 wires ending) of the selection.
 *
 * Note, wires need to be inserted as the Index is used.
 */
[[nodiscard]] auto get_inserted_cross_points(
    const CircuitData& circuit, const Selection& selection) -> std::vector<point_t>;

/**
 * @brief: Split temporary segments at the given points.
 *
 * Throws, if any segment in the selection is not temporary.
 * Throws, if any segment is partially selected.
 */
auto split_temporary_segments(CircuitData& circuit, const Selection& selection,
                              std::span<const point_t> split_points) -> void;

/**
 * @brief: Return points where temporary wires shall be split before insertion.
 *
 * Without splits wires would be colliding with endpoints of inserted wires.
 *
 * Throws, if any segment in the selection is not temporary.
 */
[[nodiscard]] auto get_temporary_selection_splitpoints(
    const CircuitData& circuit, const Selection& selection) -> std::vector<point_t>;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
