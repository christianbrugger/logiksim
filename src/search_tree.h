#ifndef LOGIKSIM_SEARCH_TREE_H
#define LOGIKSIM_SEARCH_TREE_H

#include "layout_calculation_type.h"
#include "vocabulary.h"

#include <boost/geometry.hpp>

#include <memory>
#include <ranges>

namespace logicsim {

namespace detail::search_tree {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using tree_point_t = bg::model::d2::point_xy<grid_t::value_type>;
using tree_box_t = bg::model::box<tree_point_t>;
using tree_value_t = std::pair<tree_box_t, element_id_t>;

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<tree_value_t, bgi::rstar<tree_max_node_elements>>;

auto to_rect(tree_box_t box) -> rect_t;
}  // namespace detail::search_tree

class SearchTree {
   public:
    using tree_t = detail::search_tree::tree_t;
    using value_type = detail::search_tree::tree_value_t;

   public:
    auto insert(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto remove(element_id_t element_id, layout_calculation_data_t data) -> void;
    auto update(element_id_t new_element_id, element_id_t old_element_id,
                layout_calculation_data_t data) -> void;

    auto boxes() const {
        return transform_view(tree_.begin(), tree_.end(),
                              [](const value_type &value) -> rect_t {
                                  return detail::search_tree::to_rect(value.first);
                              });
    }

   private:
    tree_t tree_ {};
};

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