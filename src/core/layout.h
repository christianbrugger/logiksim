#ifndef LOGIKSIM_LAYOUT_H
#define LOGIKSIM_LAYOUT_H

#include "algorithm/range_extended.h"
#include "component/layout/decoration_store.h"
#include "component/layout/logicitem_store.h"
#include "component/layout/wire_store.h"
#include "format/struct.h"
#include "vocabulary/insertion_mode.h"

#include <optional>
#include <utility>

namespace logicsim {

struct layout_calculation_data_t;
struct PlacedElement;

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

    // TODO make logic_items and wires simple attributes?
    [[nodiscard]] auto logic_items() -> layout::LogicItemStore &;
    [[nodiscard]] auto logic_items() const -> const layout::LogicItemStore &;
    [[nodiscard]] auto wires() -> layout::WireStore &;
    [[nodiscard]] auto wires() const -> const layout::WireStore &;
    [[nodiscard]] auto decorations() -> layout::DecorationStore &;
    [[nodiscard]] auto decorations() const -> const layout::DecorationStore &;

    [[nodiscard]] auto circuit_id() const -> circuit_id_t;

   private:
    layout::LogicItemStore logic_items_ {};
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
[[nodiscard]] auto get_inserted_segment_count(const Layout &layout) -> std::size_t;

[[nodiscard]] auto format_stats(const Layout &layout) -> std::string;

[[nodiscard]] auto format_logic_item(const Layout &layout,
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

[[nodiscard]] auto has_segments(const Layout &layout) -> bool;

[[nodiscard]] auto moved_layout(Layout layout, int delta_x,
                                int delta_y) -> std::optional<Layout>;

[[nodiscard]] auto to_layout_calculation_data(
    const Layout &layout, logicitem_id_t logicitem_id) -> layout_calculation_data_t;
[[nodiscard]] auto to_logicitem_definition(
    const Layout &layout, logicitem_id_t logicitem_id) -> LogicItemDefinition;
[[nodiscard]] auto to_decoration_definition(
    const Layout &layout, decoration_id_t decoration_id) -> DecorationDefinition;
[[nodiscard]] auto to_placed_element(const Layout &layout,
                                     logicitem_id_t logicitem_id) -> PlacedElement;

[[nodiscard]] auto get_display_states(const Layout &layout, segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t>;

[[nodiscard]] auto get_insertion_modes(const Layout &layout, segment_part_t segment_part)
    -> std::pair<InsertionMode, InsertionMode>;

[[nodiscard]] auto all_normal_display_state(const Layout &layout) -> bool;

}  // namespace logicsim

#endif
