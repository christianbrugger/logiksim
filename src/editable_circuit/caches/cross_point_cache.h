#ifndef LOGIKSIM_CROSS_POINT_CACHE_H
#define LOGIKSIM_CROSS_POINT_CACHE_H

#include "vocabulary.h"

#include <boost/geometry.hpp>

namespace logicsim {

namespace detail::cross_point_cache {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using tree_point_t = bg::model::d2::point_xy<grid_t::value_type>;
using tree_line_t = bg::model::segment<tree_point_t>;
static_assert(sizeof(tree_point_t) == 4);
static_assert(sizeof(tree_line_t) == 8);

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<tree_point_t, bgi::rstar<tree_max_node_elements>>;

auto to_tree_point(point_t point) -> tree_point_t;
auto to_tree_line(ordered_line_t line) -> tree_line_t;
}  // namespace detail::cross_point_cache

class CrossPointCache {
   public:
    using tree_t = detail::cross_point_cache::tree_t;

    auto format() const -> std::string;

    auto add_cross_point(point_t point) -> void;
    auto query_is_inside(ordered_line_t line, std::vector<point_t> &result) -> void;
    auto query_intersects(ordered_line_t line, std::vector<point_t> &result) -> void;

   private:
    tree_t tree_ {};
};

}  // namespace logicsim

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::detail::cross_point_cache::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::cross_point_cache::tree_point_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

#endif
