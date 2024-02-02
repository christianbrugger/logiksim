#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDIT_WIRE_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_EDIT_WIRE_H

#include "vocabulary/insertion_mode.h"

namespace logicsim {

struct part_t;
struct segment_t;
struct segment_part_t;
struct point_t;
struct ordered_line_t;
class Layout;

namespace editable_circuit {

struct CircuitData;

auto delete_temporary_wire_segment(CircuitData& circuit, segment_part_t& segment_part)
    -> void;

auto is_wire_position_representable(const Layout& layout, segment_part_t segment_part,
                                    int dx, int dy) -> bool;

auto move_temporary_wire_unchecked(Layout& layout, segment_t segment,
                                   part_t verify_full_part, int dx, int dy) -> void;

auto move_or_delete_temporary_wire(CircuitData& circuit, segment_part_t& segment_part,
                                   int dx, int dy) -> void;

auto change_wire_insertion_mode(CircuitData& circuit, segment_part_t& segment_part,
                                InsertionMode new_insertion_mode) -> void;

auto add_wire_segment(CircuitData& circuit, ordered_line_t line,
                      InsertionMode insertion_mode) -> segment_part_t;

auto toggle_inserted_wire_crosspoint(CircuitData& circuit, point_t point) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
