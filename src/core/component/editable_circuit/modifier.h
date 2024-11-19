#ifndef LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_MODIFIER_H
#define LOGICSIM_COMPONENT_EDITABLE_CIRCUIT_MODIFIER_H

#include "core/component/editable_circuit/circuit_data.h"
#include "core/component/editable_circuit/selection_guard.h"
#include "core/format/struct.h"
#include "core/vocabulary/insertion_mode.h"
#include "core/vocabulary/line_insertion_type.h"

#include <gsl/gsl>

namespace logicsim {

struct logicitem_id_t;
struct LogicItemDefinition;
struct point_t;
class Layout;

namespace editable_circuit {

#ifdef NDEBUG
// validation has a 17-30% performance and 50MB memory overhead
constexpr static inline auto VALIDATE_MESSAGES_DEFAULT = false;
#else
constexpr static inline auto VALIDATE_MESSAGES_DEFAULT = true;
#endif

struct ModifierConfig {
    bool enable_history {false};
    bool store_messages {false};
    bool validate_messages {VALIDATE_MESSAGES_DEFAULT};
};

/**
 * @brief: Low level circuit editing that maintains a valid layout.
 *
 * Note this class exists, so the low level methods can be direclty tested.
 *
 * Class-invariants:
 *   Logic Items:
 *      + Element body is fully representable within the grid.
 *   Inserted Logic Items:
 *      + Are not colliding with anything.
 *      + All connections with wires are compatible (type & orientation).
 *   Inserted Wires:
 *      + Segments are not colliding with anything.
 *      + Input corresponds to logicitem output and has correct orientation / position.
 *      + Wires have at least one segment.
 *      + Segments form a flat tree. With input at the root.
 *      + Have correctly set SegmentPointTypes (input, output, corner, cross, shadow).
 *   Uninserted Wires (temporary & colliding):
 *      + Have no valid parts.
 *      + Have no inputs and no outputs.
 *      + For temporary all SegmentPointTypes are shadow_point or cross_point
 *      + For colliding all SegmentPointTypes are shadow_point
 *
 *   Layout Index:
 *      + LayoutIndex is always in sync with Layout.
 *   Selections:
 *      + All Elements in all Selections of the SelectionStore are present in Layout.
 *      + Elements in Visible Selection are present in Layout.
 *   Message Validator:
 *      + If validator is set, layout matches validator state.
 **/
class Modifier {
   public:
    [[nodiscard]] explicit Modifier();
    [[nodiscard]] explicit Modifier(Layout&& layout, ModifierConfig config = {});

    [[nodiscard]] auto operator==(const Modifier&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto circuit_data() const -> const CircuitData&;
    [[nodiscard]] auto extract_layout() -> Layout;
    // TODO: write test for this one
    [[nodiscard]] auto config() const -> ModifierConfig;

    //
    // Undo & Redo
    //

    auto enable_history() -> void;
    auto disable_history() -> void;

    auto undo_group() -> void;
    auto redo_group() -> void;

    auto finish_undo_group() -> void;
    auto reopen_undo_group() -> void;

    [[nodiscard]] auto is_history_enabled() const -> bool;
    [[nodiscard]] auto has_undo() const -> bool;
    [[nodiscard]] auto has_redo() const -> bool;
    [[nodiscard]] auto has_ungrouped_undo_entries() const -> bool;

    //
    // Logic Items
    //

    auto delete_temporary_logicitem(logicitem_id_t& logicitem_id) -> void;

    auto move_temporary_logicitem_unchecked(logicitem_id_t logicitem_id,
                                            move_delta_t delta) -> void;

    auto move_or_delete_temporary_logicitem(logicitem_id_t& logicitem_id,
                                            move_delta_t delta) -> void;

    auto change_logicitem_insertion_mode(logicitem_id_t& logicitem_id,
                                         InsertionMode new_insertion_mode) -> void;

    auto add_logicitem(LogicItemDefinition&& definition, point_t position,
                       InsertionMode insertion_mode) -> logicitem_id_t;

    auto toggle_inverter(point_t point) -> void;

    auto set_attributes(logicitem_id_t logicitem_id,
                        attributes_clock_generator_t attrs) -> void;

    //
    // Decorations
    //

    auto delete_temporary_decoration(decoration_id_t& decoration_id) -> void;

    auto move_temporary_decoration_unchecked(decoration_id_t decoration_id,
                                             move_delta_t delta) -> void;

    auto move_or_delete_temporary_decoration(decoration_id_t& decoration_id,
                                             move_delta_t delta) -> void;

    auto change_decoration_insertion_mode(decoration_id_t& decoration_id,
                                          InsertionMode new_insertion_mode) -> void;

    auto add_decoration(DecorationDefinition&& definition, point_t position,
                        InsertionMode insertion_mode) -> decoration_id_t;

    auto set_attributes(decoration_id_t decoration_id,
                        attributes_text_element_t attrs) -> void;

    //
    // Wires
    //

    auto delete_temporary_wire_segment(segment_part_t& segment_part) -> void;

    auto add_wire_segment(ordered_line_t line,
                          InsertionMode insertion_mode) -> segment_part_t;

    auto change_wire_insertion_mode(segment_part_t& segment_part,
                                    InsertionMode new_insertion_mode) -> void;

    auto move_temporary_wire_unchecked(segment_part_t full_segment_part,
                                       move_delta_t delta) -> void;

    auto move_or_delete_temporary_wire(segment_part_t& segment_part,
                                       move_delta_t delta) -> void;

    auto toggle_wire_crosspoint(point_t point) -> void;

    //
    // Wire Normalization
    //

    auto set_temporary_endpoints(segment_t segment, endpoints_t endpoints) -> void;

    auto merge_uninserted_segment(segment_t segment_0, segment_t segment_1,
                                  bool restore_segment_0_key = false) -> segment_t;

    auto regularize_temporary_selection(
        const Selection& selection,
        std::optional<std::vector<point_t>> true_cross_points) -> std::vector<point_t>;

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

    auto add_to_selection(selection_id_t selection_id,
                          logicitem_id_t logicitem_id) -> void;
    auto add_to_selection(selection_id_t selection_id,
                          decoration_id_t decoration_id) -> void;
    auto add_to_selection(selection_id_t selection_id,
                          segment_part_t segment_part) -> void;
    auto remove_from_selection(selection_id_t selection_id,
                               logicitem_id_t logicitem_id) -> void;
    auto remove_from_selection(selection_id_t selection_id,
                               decoration_id_t decoration_id) -> void;
    auto remove_from_selection(selection_id_t selection_id,
                               segment_part_t segment_part) -> void;

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
    CircuitData circuit_data_;
};

static_assert(std::regular<Modifier>);

//
// Selection Guard
//

using ModifierSelectionGuard = SelectionGuardTemplate<Modifier>;

//
// Free Methods
//

/**
 * @brief: Check the class-invariants manually, e.g. for tests.
 *
 * Checking the invariants is extremely expensive compared and needs to be
 * enabled with DEBUG_CHECK_CLASS_INVARIANTS on a per method level.
 */
[[nodiscard]] auto is_valid(const Modifier& modifier) -> bool;

[[nodiscard]] auto get_inserted_cross_points(
    const Modifier& modifier, const Selection& selection) -> std::vector<point_t>;

[[nodiscard]] auto get_temporary_selection_splitpoints(
    const Modifier& modifier, const Selection& selection) -> std::vector<point_t>;

//
// Selection Based
//

auto add_wire_segments(Modifier& modifier, point_t p0, point_t p1,
                       LineInsertionType segment_type, InsertionMode insertion_mode,
                       selection_id_t selection_id = null_selection_id) -> void;

auto change_insertion_mode_consuming(Modifier& modifier, selection_id_t selection_id,
                                     InsertionMode new_insertion_mode) -> void;

auto new_positions_representable(const Layout& Layout, const Selection& selection,
                                 move_delta_t delta) -> bool;

auto move_temporary_unchecked(Modifier& modifier, const Selection& selection,
                              move_delta_t delta) -> void;

auto move_or_delete_temporary_consuming(Modifier& modifier, selection_id_t selection_id,
                                        move_delta_t delta) -> void;

auto delete_all(Modifier& modifier, selection_id_t selection_id) -> void;

}  // namespace editable_circuit

}  // namespace logicsim

#endif
