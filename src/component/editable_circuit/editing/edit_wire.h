#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDITING_EDIT_WIRE_H

#include "vocabulary/insertion_mode.h"
#include "vocabulary/point.h"

#include <optional>
#include <span>
#include <vector>

namespace logicsim {

struct part_t;
struct segment_t;
struct segment_part_t;
struct point_t;
struct ordered_line_t;
class Layout;
class Selection;

namespace editable_circuit {

struct CircuitData;

namespace editing {

auto delete_temporary_wire_segment(CircuitData& circuit,
                                   segment_part_t& segment_part) -> void;

[[nodiscard]] auto is_wire_position_representable(const Layout& layout,
                                                  segment_part_t segment_part, int dx,
                                                  int dy) -> bool;

[[nodiscard]] auto new_wire_positions_representable(const Layout& layout,
                                                    const Selection& selection,
                                                    int delta_x, int delta_y) -> bool;

// TODO why verify_full_part ??
auto move_temporary_wire_unchecked(Layout& layout, segment_t segment,
                                   part_t verify_full_part, int dx, int dy) -> void;

auto move_or_delete_temporary_wire(CircuitData& circuit, segment_part_t& segment_part,
                                   int dx, int dy) -> void;

auto change_wire_insertion_mode(CircuitData& circuit, segment_part_t& segment_part,
                                InsertionMode new_mode) -> void;

auto add_wire_segment(CircuitData& circuit, ordered_line_t line,
                      InsertionMode insertion_mode) -> segment_part_t;

auto toggle_wire_crosspoint(CircuitData& circuit, point_t point) -> void;

//
// Regularization
//

auto regularize_temporary_selection(CircuitData& circuit, const Selection& selection,
                                    std::optional<std::vector<point_t>> true_cross_points)
    -> std::vector<point_t>;

[[nodiscard]] auto get_inserted_cross_points(
    const CircuitData& circuit, const Selection& selection) -> std::vector<point_t>;

auto split_temporary_segments(CircuitData& circuit, const Selection& selection,
                              std::span<const point_t> split_points) -> void;

[[nodiscard]] auto get_temporary_selection_splitpoints(
    const CircuitData& circuit, const Selection& selection) -> std::vector<point_t>;

}  // namespace editing

}  // namespace editable_circuit

}  // namespace logicsim

#endif
