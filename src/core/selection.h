#ifndef LOGICSIM_SELECTION_H
#define LOGICSIM_SELECTION_H

#include "core/layout_message_forward.h"
#include "core/part_selection.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/display_state_map.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/segment.h"

#include <ankerl/unordered_dense.h>

#include <optional>
#include <utility>  // std::pair
#include <vector>

namespace logicsim {

struct point_fine_t;
struct ordered_line_t;
struct segment_part_t;
class Layout;

namespace selection {

using logicitems_set_t = ankerl::unordered_dense::set<logicitem_id_t>;
using decorations_set_t = ankerl::unordered_dense::set<decoration_id_t>;

using map_key_t = segment_t;
using map_value_t = PartSelection;
using map_pair_t = std::pair<map_key_t, map_value_t>;

static_assert(sizeof(map_key_t) == 8);
static_assert(sizeof(map_value_t) == 10);
static_assert(sizeof(map_pair_t) == 20);

using segment_pair_t = selection::map_pair_t;
using segment_map_t = ankerl::unordered_dense::map<map_key_t, map_value_t>;

}  // namespace selection

/**
 * @brief: A selection of logicitems and segment parts of a Layout.
 *
 * Class-invariants:
 *   + stored logicitem_ids, decoration_ids and segments are not null
 *   + selected segments entries have at least one part in the PartSelection
 */
class Selection {
   public:
    using segment_pair_t = selection::segment_pair_t;

    using logicitems_set_t = selection::logicitems_set_t;
    using decorations_set_t = selection::decorations_set_t;
    using segment_map_t = selection::segment_map_t;

   public:
    [[nodiscard]] explicit Selection() = default;
    [[nodiscard]] explicit Selection(std::span<const logicitem_id_t> logicitems,
                                     std::span<const decoration_id_t> decorations);
    [[nodiscard]] explicit Selection(logicitems_set_t &&logicitems,
                                     decorations_set_t &&decorations,
                                     segment_map_t &&segments);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_info(bool as_selection = true) const -> std::string;

    [[nodiscard]] auto operator==(const Selection &) const -> bool = default;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;
    auto clear() -> void;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto add_logicitem(logicitem_id_t logicitem_id) -> void;
    auto remove_logicitem(logicitem_id_t logicitem_id) -> void;
    auto toggle_logicitem(logicitem_id_t logicitem_id) -> void;

    auto add_decoration(decoration_id_t decoration_id) -> void;
    auto remove_decoration(decoration_id_t decoration_id) -> void;

    auto add_segment(segment_part_t segment_part) -> void;
    auto remove_segment(segment_part_t segment_part) -> void;
    auto set_selection(segment_t segment, PartSelection &&parts) -> void;

    [[nodiscard]] auto is_selected(logicitem_id_t logicitem_id) const -> bool;
    [[nodiscard]] auto is_selected(decoration_id_t decoration_id) const -> bool;
    [[nodiscard]] auto is_selected(segment_t segment) const -> bool;

    [[nodiscard]] auto selected_logicitems() const -> std::span<const logicitem_id_t>;
    [[nodiscard]] auto selected_decorations() const -> std::span<const decoration_id_t>;
    [[nodiscard]] auto selected_segments() const -> std::span<const segment_pair_t>;
    [[nodiscard]] auto selected_segments(segment_t segment) const
        -> const PartSelection &;

    auto submit(const InfoMessage &message) -> void;

   private:
    auto handle(const info_message::LogicItemDeleted &message) -> void;
    auto handle(const info_message::LogicItemIdUpdated &message) -> void;

    auto handle(const info_message::DecorationDeleted &message) -> void;
    auto handle(const info_message::DecorationIdUpdated &message) -> void;

    auto handle(const info_message::SegmentIdUpdated &message) -> void;
    auto handle(const info_message::SegmentPartMoved &message) -> void;
    auto handle(const info_message::SegmentPartDeleted &message) -> void;

    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    logicitems_set_t selected_logicitems_ {};
    decorations_set_t selected_decorations_ {};
    segment_map_t selected_segments_ {};
};

//
// Free Functions
//

[[nodiscard]] auto is_valid_selection(const Selection &selection,
                                      const Layout &layout) -> bool;

//
//
//

[[nodiscard]] auto has_logicitems(const Selection &selection) -> bool;
[[nodiscard]] auto has_decorations(const Selection &selection) -> bool;
[[nodiscard]] auto get_lines(const Selection &selection,
                             const Layout &layout) -> std::vector<ordered_line_t>;
[[nodiscard]] auto all_normal_display_state(const Selection &selection,
                                            const Layout &layout) -> bool;
[[nodiscard]] auto anything_colliding(const Selection &selection,
                                      const Layout &layout) -> bool;
[[nodiscard]] auto anything_temporary(const Selection &selection,
                                      const Layout &layout) -> bool;
[[nodiscard]] auto anything_valid(const Selection &selection,
                                  const Layout &layout) -> bool;
[[nodiscard]] auto display_states(const Selection &selection,
                                  const Layout &layout) -> DisplayStateMap;

[[nodiscard]] auto is_selected(const Selection &selection, const Layout &layout,
                               segment_t segment, point_fine_t point) -> bool;

//
//
//

auto add_segment(Selection &selection, segment_t segment, const Layout &layout) -> void;
auto add_segment_tree(Selection &selection, wire_id_t wire_id,
                      const Layout &layout) -> void;

auto remove_segment(Selection &selection, segment_t segment,
                    const Layout &layout) -> void;
auto remove_segment_tree(Selection &selection, wire_id_t wire_id,
                         const Layout &layout) -> void;

auto add_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                      point_fine_t point) -> void;

auto remove_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void;

auto toggle_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void;

auto get_single_logicitem(const Selection &selection) -> logicitem_id_t;
auto get_single_decoration(const Selection &selection) -> decoration_id_t;
auto get_single_segment(const Selection &selection)
    -> std::optional<selection::segment_pair_t>;

}  // namespace logicsim

#endif
