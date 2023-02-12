#include "search_tree.h"

#include "format.h"

#include <ankerl/unordered_dense.h>
#include <boost/geometry.hpp>
#include <fmt/core.h>

#include <string>

namespace logicsim {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

struct ConnectionEntry {
    element_id_t element_id;
    connection_id_t connection_id;

    auto format() const -> std::string {
        return fmt::format("<Con: {}-{}>", element_id, connection_id);
    }
};

using tree_point_t = bg::model::d2::point_xy<grid_t::value_type>;
using tree_value_t = std::pair<tree_point_t, ConnectionEntry>;
using map_t = ankerl::unordered_dense::map<point_t, ConnectionEntry>;

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<tree_value_t, bgi::rstar<tree_max_node_elements>>;

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::tree_point_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "<{}, {}>", obj.x(), obj.y());
    }
};

template <>
struct fmt::formatter<logicsim::tree_value_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::tree_value_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}: {}", obj.first, obj.second);
    }
};

namespace logicsim {

auto test_tree() -> void {
    auto tree = tree_t {};
    auto map = map_t {};

    auto point = tree_point_t {0, 0};
    auto connection = ConnectionEntry {10, 5};
    auto value = tree_value_t {point, connection};

    tree.insert(value);
    tree.insert({{10, 10}, {10, 6}});

    map[{0, 0}] = {10, 5};
    map[{10, 10}] = {10, 6};

    auto search_point = tree_point_t {7, 7};

    std::vector<tree_t::value_type> result;
    auto count = tree.query(bgi::nearest(search_point, 5), std::back_inserter(result));

    fmt::print("rtree: {}\n", tree);
    fmt::print("query: {} {}\n", count, result);
    fmt::print("map: {}\n", map);
    fmt::print("query: {}\n", map[{0, 0}]);
}

}  // namespace logicsim