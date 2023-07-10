#ifndef LOGIKSIM_SEARCH_TREE_H
#define LOGIKSIM_SEARCH_TREE_H

#include "editable_circuit/caches/helpers.h"
#include "editable_circuit/messages.h"
#include "vocabulary.h"

#include <boost/geometry.hpp>

#include <array>
#include <memory>
#include <optional>
#include <ranges>

namespace logicsim {

class Circuit;

namespace detail::spatial_tree {
// Boost R-Tree Documentation:
// https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/spatial_indexes.html

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// TODO create new type? used elsehwere?
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
auto get_selection_box(ordered_line_t segment) -> tree_box_t;
auto to_rect(tree_box_t box) -> rect_fine_t;
auto to_box(rect_fine_t rect) -> tree_box_t;

auto operator==(const tree_t &a, const tree_t &b) -> bool;
auto operator!=(const tree_t &a, const tree_t &b) -> bool;

}  // namespace detail::spatial_tree

class SpatialTree {
   public:
    using tree_t = detail::spatial_tree::tree_t;
    using value_type = detail::spatial_tree::tree_value_t;

    using query_result_t = detail::spatial_tree::tree_payload_t;
    using queried_segments_t = std::array<segment_t, 4>;

   public:
    [[nodiscard]] auto format() const -> std::string;

    auto query_selection(rect_fine_t rect) const -> std::vector<query_result_t>;
    auto query_line_segments(point_t point) const -> queried_segments_t;

    auto rects() const {
        return transform_view(tree_.begin(), tree_.end(),
                              [](const value_type &value) -> rect_fine_t {
                                  return detail::spatial_tree::to_rect(value.first);
                              });
    }

    auto submit(editable_circuit::InfoMessage message) -> void;
    auto validate(const Circuit &circuit) const -> void;

   private:
    auto handle(editable_circuit::info_message::LogicItemInserted message) -> void;
    auto handle(editable_circuit::info_message::LogicItemUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedLogicItemIdUpdated message)
        -> void;

    auto handle(editable_circuit::info_message::SegmentInserted message) -> void;
    auto handle(editable_circuit::info_message::SegmentUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedSegmentIdUpdated message) -> void;

    tree_t tree_ {};
};

[[nodiscard]] auto get_segment_count(SpatialTree::queried_segments_t result) -> int;
[[nodiscard]] auto all_same_element_id(SpatialTree::queried_segments_t result) -> bool;
[[nodiscard]] auto get_segment_indices(SpatialTree::queried_segments_t result)
    -> std::array<segment_index_t, 4>;
[[nodiscard]] auto get_unique_element_id(SpatialTree::queried_segments_t) -> element_id_t;

}  // namespace logicsim

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::detail::spatial_tree::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::spatial_tree::tree_point_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

template <>
struct fmt::formatter<logicsim::detail::spatial_tree::tree_box_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::spatial_tree::tree_box_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.min_corner(), obj.max_corner());
    }
};

template <>
struct fmt::formatter<logicsim::detail::spatial_tree::tree_value_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::spatial_tree::tree_value_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}: {}", obj.first, obj.second);
    }
};

#endif