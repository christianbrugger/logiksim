#ifndef LOGIKSIM_EDITABLE_CIRCUIT_SELECTION_H
#define LOGIKSIM_EDITABLE_CIRCUIT_SELECTION_H

#include "editable_circuit/message_forward.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <folly/small_vector.h>

namespace logicsim {

class Layout;

namespace detail::selection {

using map_key_t = segment_t;
using policy = folly::small_vector_policy::policy_size_type<uint16_t>;
using map_value_t = folly::small_vector<part_t, 3, policy>;
using map_pair_t = std::pair<map_key_t, map_value_t>;

static_assert(sizeof(map_key_t) == 8);
static_assert(sizeof(map_value_t) == 14);
static_assert(sizeof(map_pair_t) == 24);

using logicitems_set_t = ankerl::unordered_dense::set<element_id_t>;
using segment_map_t = ankerl::unordered_dense::map<map_key_t, map_value_t>;
}  // namespace detail::selection

class Selection;

[[nodiscard]] auto has_logic_items(const Selection &selection) -> bool;
[[nodiscard]] auto get_lines(const Selection &selection, const Layout &layout)
    -> std::vector<ordered_line_t>;
[[nodiscard]] auto anything_colliding(const Selection &selection, const Layout &layout)
    -> bool;

[[nodiscard]] auto is_selected(const Selection &selection, const Layout &layout,
                               segment_t segment, point_fine_t point) -> bool;

class Selection {
   public:
    using segment_pair_t = detail::selection::map_pair_t;
    using part_vector_t = detail::selection::map_value_t;

    auto swap(Selection &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto empty() const noexcept -> bool;
    auto clear() -> void;

    auto add_logicitem(element_id_t element_id) -> void;
    auto remove_logicitem(element_id_t element_id) -> void;
    auto toggle_logicitem(element_id_t element_id) -> void;

    auto add_segment(segment_part_t segment_part) -> void;
    auto remove_segment(segment_part_t segment_part) -> void;
    auto set_selection(segment_t segment, part_vector_t &&parts) -> void;

    [[nodiscard]] auto is_selected(element_id_t element_id) const -> bool;
    [[nodiscard]] auto is_selected(segment_t segment) const -> bool;

    [[nodiscard]] auto selected_logic_items() const -> std::span<const element_id_t>;
    [[nodiscard]] auto selected_segments() const -> std::span<const segment_pair_t>;
    [[nodiscard]] auto selected_segments(segment_t segment) const
        -> std::span<const part_t>;

    auto submit(editable_circuit::InfoMessage message) -> void;
    auto validate(const Layout &layout) const -> void;

   private:
    auto handle(editable_circuit::info_message::LogicItemDeleted message) -> void;
    auto handle(editable_circuit::info_message::LogicItemIdUpdated message) -> void;

    auto handle(editable_circuit::info_message::SegmentIdUpdated message) -> void;
    auto handle(editable_circuit::info_message::SegmentPartMoved message) -> void;
    auto handle(editable_circuit::info_message::SegmentPartDeleted message) -> void;

    detail::selection::logicitems_set_t selected_logicitems_ {};
    detail::selection::segment_map_t selected_segments_ {};
};

auto swap(Selection &a, Selection &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void;

namespace logicsim {

// [&](part_t part, bool selected){}
template <typename Func>
auto iter_parts(part_t full_part, Selection::part_vector_t parts, Func func) {
    offset_t pivot = full_part.begin;
    std::ranges::sort(parts);

    for (auto part : parts) {
        if (pivot != part.begin) {
            func(part_t {pivot, part.begin}, false);
        }
        func(part, true);
        pivot = part.end;
    }

    if (pivot != full_part.end) {
        func(part_t {pivot, full_part.end}, false);
    }
}

// [&](part_t part, bool selected){}
template <typename Func>
auto iter_parts(part_t full_part, std::span<const part_t> const_parts, Func func) {
    iter_parts(full_part,
               Selection::part_vector_t {const_parts.begin(), const_parts.end()}, func);
}

auto add_segment(Selection &selection, segment_t segment, const Layout &layout) -> void;
auto add_segment_tree(Selection &selection, element_id_t element_id, const Layout &layout)
    -> void;

auto remove_segment(Selection &selection, segment_t segment, const Layout &layout)
    -> void;
auto remove_segment_tree(Selection &selection, element_id_t element_id,
                         const Layout &layout) -> void;

auto add_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                      point_fine_t point) -> void;

auto remove_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void;

auto toggle_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void;

}  // namespace logicsim

#endif
