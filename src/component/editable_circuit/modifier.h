#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_MODIFIER_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_MODIFIER_H

#include "circuit_data.h"
#include "component/editable_circuit/selection_guard.h"
#include "format/struct.h"
#include "vocabulary/insertion_mode.h"
#include "vocabulary/line_insertion_type.h"

#include <gsl/gsl>

namespace logicsim {

struct logicitem_id_t;
struct LogicItemDefinition;
struct point_t;
class Layout;

namespace editable_circuit {

enum class ModifierLogging {
    disabled,
    enabled,
};

/**
 * @brief: Low level circuit editing that maintains a valid layout.
 *
 * Class-invariants:
 *   Logic Items:
 *      + Body is fully representable within the grid.
 *   Inserted Logic Items:
 *      + Are not colliding with anything.
 *      + All connections with wires are compatible (type & orientation).
 *   Inserted Wires:
 *      + Segments are not colliding with anything.
 *      + Segments form a flat tree. With input at the root.
 *      + Have either zero or one input.
 *      + Input corresponds to logicitem output and has correct orientation / position.
 *      + Have correctly set SegmentPointTypes (input, output, corner, cross, shadow).
 *   Uninserted Wires:
 *      + Have no valid parts.
 *      + Have no inputs or outputs.
 *      + All SegmentPointTypes are shadow_point
 *
 *   Layout Index:
 *      + LayoutIndex is always in sync with Layout.
 *   Selections:
 *      + All Elements in all Selections of the SelectionStore are present in Layout.
 *      + Elements in Visible Selection are present in Layout.
 **/
class Modifier {
   public:
    [[nodiscard]] explicit Modifier() = default;
    [[nodiscard]] explicit Modifier(ModifierLogging logging);
    [[nodiscard]] explicit Modifier(Layout&& layout);
    [[nodiscard]] explicit Modifier(Layout&& layout, ModifierLogging logging);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto circuit_data() const -> const CircuitData&;
    [[nodiscard]] auto extract_layout() -> Layout;

    //
    // Logic Items
    //

    auto delete_temporary_logicitem(logicitem_id_t& logicitem_id,
                                    logicitem_id_t* preserve_element = nullptr) -> void;

    auto move_temporary_logicitem_unchecked(const logicitem_id_t logicitem_id, int dx,
                                            int dy) -> void;

    auto move_or_delete_temporary_logicitem(logicitem_id_t& logicitem_id, int dx, int dy)
        -> void;

    auto change_logicitem_insertion_mode(logicitem_id_t& logicitem_id,
                                         InsertionMode new_insertion_mode) -> void;

    auto add_logicitem(const LogicItemDefinition& definition, point_t position,
                       InsertionMode insertion_mode) -> logicitem_id_t;

    auto toggle_inverter(point_t point) -> void;

    auto set_attributes(logicitem_id_t logicitem_id, attributes_clock_generator_t attrs)
        -> void;

    //
    // Wires
    //

    auto delete_temporary_wire_segment(segment_part_t& segment_part) -> void;

    auto add_wire_segment(ordered_line_t line, InsertionMode insertion_mode)
        -> segment_part_t;

    auto change_wire_insertion_mode(segment_part_t& segment_part,
                                    InsertionMode new_insertion_mode) -> void;

    auto move_temporary_wire_unchecked(segment_t segment, part_t verify_full_part, int dx,
                                       int dy) -> void;

    auto move_or_delete_temporary_wire(segment_part_t& segment_part, int dx, int dy)
        -> void;

    auto toggle_wire_crosspoint(point_t point) -> void;

    //
    // Wire Normalization
    //

    auto regularize_temporary_selection(
        const Selection& selection, std::optional<std::vector<point_t>> true_cross_points)
        -> std::vector<point_t>;

    auto split_temporary_segments(const Selection& selection,
                                  std::span<const point_t> split_points) -> void;

    //
    // Selections
    //

    [[nodiscard]] auto create_selection() -> selection_id_t;
    [[nodiscard]] auto create_selection(Selection selection) -> selection_id_t;
    [[nodiscard]] auto create_selection(selection_id_t copy_id) -> selection_id_t;
    auto destroy_selection(selection_id_t selection_id) -> void;
    auto set_selection(selection_id_t selection_id, Selection selection) -> void;

    auto add_to_selection(selection_id_t, logicitem_id_t logicitem_id) -> void;
    auto add_to_selection(selection_id_t, segment_part_t segment_part) -> void;
    auto remove_from_selection(selection_id_t, logicitem_id_t logicitem_id) -> void;
    auto remove_from_selection(selection_id_t, segment_part_t segment_part) -> void;

    //
    // Visible Selection
    //

    auto clear_visible_selection() -> void;
    auto set_visible_selection(Selection selection) -> void;

    auto add_visible_selection_rect(SelectionFunction function, rect_fine_t rect) -> void;
    auto try_pop_last_visible_selection_rect() -> bool;
    auto try_update_last_visible_selection_rect(rect_fine_t rect) -> bool;
    auto apply_all_visible_selection_operations() -> void;

   private:
    CircuitData circuit_data_ {};
};

//
// Selection Guard
//

using ModifierSelectionGuard = SelectionGuardTemplate<Modifier>;

//
// Free Methods
//

[[nodiscard]] auto get_inserted_cross_points(const Modifier& modifier,
                                             const Selection& selection)
    -> std::vector<point_t>;

[[nodiscard]] auto get_temporary_selection_splitpoints(const Modifier& modifier,
                                                       const Selection& selection)
    -> std::vector<point_t>;

//
// Selection Based
//

auto add_wire_segments(Modifier& modifier, point_t p0, point_t p1,
                       LineInsertionType segment_type, InsertionMode insertion_mode,
                       selection_id_t selection_id = null_selection_id) -> void;

auto change_insertion_mode_consuming(Modifier& modifier, selection_id_t selection_id,
                                     InsertionMode new_insertion_mode) -> void;

auto new_positions_representable(const Layout& Layout, const Selection& selection,
                                 int delta_x, int delta_y) -> bool;

// TODO move checks to low-level method
auto move_temporary_unchecked(Modifier& modifier, const Selection& selection, int delta_x,
                              int delta_y) -> void;

auto move_or_delete_temporary_consuming(Modifier& modifier, selection_id_t selection_id,
                                        int delta_x, int delta_y) -> void;

auto delete_all(Modifier& modifier, selection_id_t selection_id) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
