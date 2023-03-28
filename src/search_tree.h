#ifndef LOGIKSIM_SEARCH_TREE_H
#define LOGIKSIM_SEARCH_TREE_H

#include "layout_calculation_type.h"
#include "vocabulary.h"

#include <boost/geometry.hpp>

#include <array>
#include <memory>
#include <optional>
#include <ranges>

namespace logicsim {

namespace detail::search_tree {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

struct tree_payload_t {
    element_id_t element_id {null_element};
    segment_index_t segment_index {null_segment_index};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const tree_payload_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const tree_payload_t &other) const = default;
};

using tree_point_t = bg::model::d2::point_xy<grid_fine_t>;
using tree_box_t = bg::model::box<tree_point_t>;
using tree_value_t = std::pair<tree_box_t, tree_payload_t>;

static_assert(sizeof(tree_box_t) == 32);
static_assert(sizeof(tree_value_t) == 40);

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<tree_value_t, bgi::rstar<tree_max_node_elements>>;

auto get_selection_box(layout_calculation_data_t data) -> tree_box_t;
auto get_selection_box(line_t segment) -> tree_box_t;
auto to_rect(tree_box_t box) -> rect_fine_t;
auto to_box(rect_fine_t rect) -> tree_box_t;
}  // namespace detail::search_tree

class SearchTree {
   public:
    using tree_t = detail::search_tree::tree_t;
    using value_type = detail::search_tree::tree_value_t;

    using query_result_t = detail::search_tree::tree_payload_t;
    using queried_segments_t = std::array<segment_t, 4>;

   public:
    auto insert(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto remove(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto update(element_id_t new_element_id, element_id_t old_element_id,
                layout_calculation_data_t data) -> void;

    auto insert(element_id_t element_id, line_t segment, segment_index_t index) -> void;
    auto remove(element_id_t element_id, line_t segment, segment_index_t index) -> void;

    auto query_selection(rect_fine_t rect) const -> std::vector<query_result_t>;
    auto query_line_segments(point_t point) const -> queried_segments_t;

    auto rects() const {
        return transform_view(tree_.begin(), tree_.end(),
                              [](const value_type &value) -> rect_fine_t {
                                  return detail::search_tree::to_rect(value.first);
                              });
    }

   private:
    tree_t tree_ {};
};

auto get_segment_count(SearchTree::queried_segments_t result) -> int;
auto all_same_element_id(SearchTree::queried_segments_t result) -> bool;
auto get_unique_element_id(SearchTree::queried_segments_t) -> element_id_t;

}  // namespace logicsim

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::detail::search_tree::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::search_tree::tree_point_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

template <>
struct fmt::formatter<logicsim::detail::search_tree::tree_box_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::search_tree::tree_box_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.min_corner(), obj.max_corner());
    }
};

template <>
struct fmt::formatter<logicsim::detail::search_tree::tree_value_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::search_tree::tree_value_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}: {}", obj.first, obj.second);
    }
};

#endif