#ifndef LOGICSIM_SELECTION_H
#define LOGICSIM_SELECTION_H

#include "layout_message_forward.h"
#include "part_selection.h"
#include "vocabulary/display_state_map.h"
#include "vocabulary/logicitem_id.h"
#include "vocabulary/segment.h"

#include <ankerl/unordered_dense.h>

#include <utility>  // std::pair
#include <vector>

namespace logicsim {

struct point_fine_t;
struct ordered_line_t;
struct segment_part_t;
class Layout;

namespace detail::selection {

using logicitems_set_t = ankerl::unordered_dense::set<logicitem_id_t>;

using map_key_t = segment_t;
using map_value_t = PartSelection;
using map_pair_t = std::pair<map_key_t, map_value_t>;

static_assert(sizeof(map_key_t) == 8);
static_assert(sizeof(map_value_t) == 10);
static_assert(sizeof(map_pair_t) == 20);

using segment_map_t = ankerl::unordered_dense::map<map_key_t, map_value_t>;

}  // namespace detail::selection

// TODO rename to TrackedElements
class Selection;

[[nodiscard]] auto has_logic_items(const Selection &selection) -> bool;
[[nodiscard]] auto get_lines(const Selection &selection, const Layout &layout)
    -> std::vector<ordered_line_t>;
[[nodiscard]] auto all_normal_display_state(const Selection &selection,
                                            const Layout &layout) -> bool;
[[nodiscard]] auto anything_colliding(const Selection &selection, const Layout &layout)
    -> bool;
[[nodiscard]] auto anything_temporary(const Selection &selection, const Layout &layout)
    -> bool;
[[nodiscard]] auto anything_valid(const Selection &selection, const Layout &layout)
    -> bool;
[[nodiscard]] auto display_states(const Selection &selection, const Layout &layout)
    -> DisplayStateMap;

[[nodiscard]] auto is_selected(const Selection &selection, const Layout &layout,
                               segment_t segment, point_fine_t point) -> bool;

class Selection {
   public:
    using segment_pair_t = detail::selection::map_pair_t;

    auto swap(Selection &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_info(bool as_selection = true) const -> std::string;

    [[nodiscard]] auto operator==(const Selection &) const -> bool = default;

    [[nodiscard]] auto empty() const noexcept -> bool;
    auto clear() -> void;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto add(logicitem_id_t logicitem_id) -> void;
    auto remove_logicitem(logicitem_id_t logicitem_id) -> void;
    auto toggle_logicitem(logicitem_id_t logicitem_id) -> void;

    auto add_segment(segment_part_t segment_part) -> void;
    auto remove_segment(segment_part_t segment_part) -> void;
    auto set_selection(segment_t segment, PartSelection &&parts) -> void;

    [[nodiscard]] auto is_selected(logicitem_id_t logicitem_id) const -> bool;
    [[nodiscard]] auto is_selected(segment_t segment) const -> bool;

    [[nodiscard]] auto selected_logic_items() const -> std::span<const logicitem_id_t>;
    [[nodiscard]] auto selected_segments() const -> std::span<const segment_pair_t>;
    [[nodiscard]] auto selected_segments(segment_t segment) const
        -> const PartSelection &;

    auto submit(const editable_circuit::InfoMessage &message) -> void;
    auto validate(const Layout &layout) const -> void;

   private:
    auto handle(const editable_circuit::info_message::LogicItemDeleted &message) -> void;
    auto handle(const editable_circuit::info_message::LogicItemIdUpdated &message)
        -> void;

    auto handle(const editable_circuit::info_message::SegmentIdUpdated &message) -> void;
    auto handle(const editable_circuit::info_message::SegmentPartMoved &message) -> void;
    auto handle(const editable_circuit::info_message::SegmentPartDeleted &message)
        -> void;

    detail::selection::logicitems_set_t selected_logicitems_ {};
    detail::selection::segment_map_t selected_segments_ {};
};

auto swap(Selection &a, Selection &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void;

namespace logicsim {

auto add_segment(Selection &selection, segment_t segment, const Layout &layout) -> void;
auto add_segment_tree(Selection &selection, wire_id_t wire_id, const Layout &layout)
    -> void;

auto remove_segment(Selection &selection, segment_t segment, const Layout &layout)
    -> void;
auto remove_segment_tree(Selection &selection, wire_id_t wire_id, const Layout &layout)
    -> void;

auto add_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                      point_fine_t point) -> void;

auto remove_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void;

auto toggle_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void;

}  // namespace logicsim

#endif
