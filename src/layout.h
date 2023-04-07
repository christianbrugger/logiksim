#ifndef LOGIKSIM_LAYOUT_H
#define LOGIKSIM_LAYOUT_H

#include "line_tree.h"
#include "range.h"
#include "segment_tree.h"
#include "vocabulary.h"

#include <fmt/core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace logicsim {

class Layout;

[[nodiscard]] auto is_inserted(const Layout &layout, element_id_t element_id) -> bool;

[[nodiscard]] auto get_segment_info(const Layout &layout, segment_t segment)
    -> segment_info_t;
[[nodiscard]] auto get_line(const Layout &layout, segment_t segment) -> ordered_line_t;
[[nodiscard]] auto get_line(const Layout &layout, segment_part_t segment_part)
    -> ordered_line_t;

class Layout {
   public:
    [[nodiscard]] explicit Layout() = default;
    [[nodiscard]] explicit Layout(circuit_id_t circuit_id);

    auto swap(Layout &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_element(element_id_t element_id) const -> std::string;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto element_count() const -> std::size_t;

    // TODO make add_default_element private when not needed anymore
    auto add_default_element() -> element_id_t;

    auto add_placeholder(display_state_t display_state) -> element_id_t;
    // TODO rework these methods
    auto add_line_tree(display_state_t display_state = display_state_t::normal)
        -> element_id_t;
    auto add_logic_element(point_t position,
                           orientation_t orientation = orientation_t::undirected,
                           display_state_t display_state = display_state_t::normal,
                           color_t color = defaults::color_black) -> element_id_t;

    // swaps the element with last one and deletes it, returns deleted id
    auto swap_and_delete_element(element_id_t element_id) -> element_id_t;
    auto swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void;

    // TODO remove line tree, when not needed anymore
    auto set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void;
    auto set_position(element_id_t element_id, point_t point) -> void;
    auto set_display_state(element_id_t element_id, display_state_t display_state)
        -> void;

    [[nodiscard]] auto element_ids() const noexcept -> forward_range_t<element_id_t>;

    [[nodiscard]] auto circuit_id() const noexcept -> circuit_id_t;
    [[nodiscard]] auto segment_tree(element_id_t element_id) const -> const SegmentTree &;
    [[nodiscard]] auto line_tree(element_id_t element_id) const -> const LineTree &;
    [[nodiscard]] auto position(element_id_t element_id) const -> point_t;
    [[nodiscard]] auto orientation(element_id_t element_id) const -> orientation_t;
    [[nodiscard]] auto display_state(element_id_t element_id) const -> display_state_t;
    [[nodiscard]] auto color(element_id_t element_id) const -> color_t;

    [[nodiscard]] auto modifyable_segment_tree(element_id_t element_id) -> SegmentTree &;

    auto validate() const -> void;

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2) -> void;
    auto delete_last_element() -> void;

    std::vector<SegmentTree> segment_trees_ {};
    std::vector<LineTree> line_trees_ {};
    std::vector<point_t> positions_ {};
    std::vector<orientation_t> orientation_ {};
    std::vector<display_state_t> display_states_ {};
    std::vector<color_t> colors_ {};

    circuit_id_t circuit_id_ {0};
};

auto swap(Layout &a, Layout &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Layout &a, logicsim::Layout &b) noexcept -> void;

#endif