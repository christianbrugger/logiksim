#ifndef LOGIKSIM_LAYOUT_H
#define LOGIKSIM_LAYOUT_H

#include "core/algorithm/range_extended.h"
#include "core/component/layout/decoration_store.h"
#include "core/component/layout/logicitem_store.h"
#include "core/component/layout/wire_store.h"
#include "core/format/struct.h"
#include "core/vocabulary/insertion_mode.h"

#include <optional>
#include <utility>

namespace logicsim {

struct layout_calculation_data_t;
struct decoration_layout_data_t;
struct PlacedLogicItem;
struct PlacedDecoration;

/**
 * @brief: The layout is the visual representation of the circuit, consisting of
 *         logic items and wires.
 *
 * Class-invariants:
 *     + See those of `LogicItemStore` and `WireStore`
 */
class Layout {
   public:
    [[nodiscard]] explicit Layout() = default;
    [[nodiscard]] explicit Layout(circuit_id_t circuit_id);

    /**
     * @brief: brings the store in canonical form,
     *         so that visual equivalent layouts compare equal
     */
    auto normalize() -> void;
    [[nodiscard]] auto operator==(const Layout &) const -> bool = default;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto format() const -> std::string;

    // TODO make logicitems and wires simple attributes?
    // TODO rename to logicitems
    [[nodiscard]] auto logicitems() -> layout::LogicItemStore &;
    [[nodiscard]] auto logicitems() const -> const layout::LogicItemStore &;
    [[nodiscard]] auto wires() -> layout::WireStore &;
    [[nodiscard]] auto wires() const -> const layout::WireStore &;
    [[nodiscard]] auto decorations() -> layout::DecorationStore &;
    [[nodiscard]] auto decorations() const -> const layout::DecorationStore &;

    [[nodiscard]] auto circuit_id() const -> circuit_id_t;

   private:
    layout::LogicItemStore logicitems_ {};
    layout::WireStore wires_ {};
    layout::DecorationStore decorations_ {};

    circuit_id_t circuit_id_ {0};
};

//
// Public Functions
//

[[nodiscard]] auto logicitem_ids(const Layout &layout)
    -> range_extended_t<logicitem_id_t>;
[[nodiscard]] auto wire_ids(const Layout &layout) -> range_extended_t<wire_id_t>;
[[nodiscard]] auto decoration_ids(const Layout &layout)
    -> range_extended_t<decoration_id_t>;

[[nodiscard]] auto inserted_wire_ids(const Layout &layout) -> range_extended_t<wire_id_t>;

[[nodiscard]] auto is_id_valid(logicitem_id_t logicitem_id, const Layout &layout) -> bool;
[[nodiscard]] auto is_id_valid(wire_id_t wire_id, const Layout &layout) -> bool;
[[nodiscard]] auto is_id_valid(decoration_id_t decoration_id,
                               const Layout &layout) -> bool;
[[nodiscard]] auto is_segment_valid(segment_t segment, const Layout &layout) -> bool;
[[nodiscard]] auto is_segment_part_valid(segment_part_t segment_part,
                                         const Layout &layout) -> bool;

[[nodiscard]] auto get_uninserted_logicitem_count(const Layout &layout) -> std::size_t;
[[nodiscard]] auto get_inserted_logicitem_count(const Layout &layout) -> std::size_t;

[[nodiscard]] auto get_uninserted_decoration_count(const Layout &layout) -> std::size_t;
[[nodiscard]] auto get_inserted_decoration_count(const Layout &layout) -> std::size_t;

[[nodiscard]] auto get_segment_count(const Layout &layout) -> std::size_t;
[[nodiscard]] auto get_temporary_segment_count(const Layout &layout) -> std::size_t;
[[nodiscard]] auto get_colliding_segment_count(const Layout &layout) -> std::size_t;
[[nodiscard]] auto get_inserted_segment_count(const Layout &layout) -> std::size_t;

[[nodiscard]] auto format_stats(const Layout &layout) -> std::string;

[[nodiscard]] auto format_logicitem(const Layout &layout,
                                    logicitem_id_t logicitem_id) -> std::string;
[[nodiscard]] auto format_wire(const Layout &layout, wire_id_t wire_id) -> std::string;
[[nodiscard]] auto format_decoration(const Layout &layout,
                                     decoration_id_t decoration_id) -> std::string;

[[nodiscard]] auto is_inserted(const Layout &layout, logicitem_id_t logicitem_id) -> bool;
[[nodiscard]] auto is_inserted(const Layout &layout,
                               decoration_id_t decoration_id) -> bool;

[[nodiscard]] auto is_wire_empty(const Layout &layout, wire_id_t wire_id) -> bool;

[[nodiscard]] auto get_segment_info(const Layout &layout,
                                    segment_t segment) -> segment_info_t;

[[nodiscard]] auto get_segment_point_type(const Layout &layout, segment_t segment,
                                          point_t position) -> SegmentPointType;

[[nodiscard]] auto get_segment_valid_parts(const Layout &layout,
                                           segment_t segment) -> const PartSelection &;

[[nodiscard]] auto get_line(const Layout &layout, segment_t segment) -> ordered_line_t;
[[nodiscard]] auto get_line(const Layout &layout,
                            segment_part_t segment_part) -> ordered_line_t;

[[nodiscard]] auto get_part(const Layout &layout, segment_t segment) -> part_t;
[[nodiscard]] auto get_segment_part(const Layout &layout,
                                    segment_t segment) -> segment_part_t;

[[nodiscard]] auto has_segments(const Layout &layout) -> bool;

[[nodiscard]] auto is_full_segment(const Layout &layout,
                                   segment_part_t segment_part) -> bool;

[[nodiscard]] auto moved_layout(Layout layout, int delta_x,
                                int delta_y) -> std::optional<Layout>;

[[nodiscard]] auto to_layout_calculation_data(
    const Layout &layout, logicitem_id_t logicitem_id) -> layout_calculation_data_t;
[[nodiscard]] auto to_decoration_layout_data(
    const Layout &layout, decoration_id_t decoration_id) -> decoration_layout_data_t;
[[nodiscard]] auto to_logicitem_definition(
    const Layout &layout, logicitem_id_t logicitem_id) -> LogicItemDefinition;
[[nodiscard]] auto to_decoration_definition(
    const Layout &layout, decoration_id_t decoration_id) -> DecorationDefinition;
[[nodiscard]] auto to_placed_logicitem(const Layout &layout,
                                       logicitem_id_t logicitem_id) -> PlacedLogicItem;
[[nodiscard]] auto to_placed_decoration(
    const Layout &layout, decoration_id_t decoration_id) -> PlacedDecoration;

[[nodiscard]] auto get_display_states(const Layout &layout, segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t>;

[[nodiscard]] auto get_insertion_modes(const Layout &layout, segment_part_t segment_part)
    -> std::pair<InsertionMode, InsertionMode>;

[[nodiscard]] auto all_normal_display_state(const Layout &layout) -> bool;

[[nodiscard]] auto get_normalized(Layout layout) -> Layout;

[[nodiscard]] auto are_normalized_equal(Layout layout1, Layout layout2) -> bool;

}  // namespace logicsim

#endif
