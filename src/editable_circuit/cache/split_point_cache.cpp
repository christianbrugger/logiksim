#include "editable_circuit/cache/split_point_cache.h"

#include "layout.h"

#include <boost/geometry.hpp>

namespace logicsim::detail::split_point_cache {
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

struct tree_container {
    tree_t value;
};
}  // namespace logicsim::detail::split_point_cache

template <>
struct fmt::formatter<logicsim::detail::split_point_cache::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::split_point_cache::tree_point_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

namespace logicsim {

namespace detail::split_point_cache {

auto to_tree_point(point_t point) -> tree_point_t {
    return tree_point_t {point.x.value, point.y.value};
}

auto to_tree_line(ordered_line_t line) -> tree_line_t {
    return tree_line_t {to_tree_point(line.p0), to_tree_point(line.p1)};
}

}  // namespace detail::split_point_cache

SplitPointCache::SplitPointCache()
    : tree_ {std::make_unique<detail::split_point_cache::tree_container>()} {}

namespace {
auto make_tree(std::span<const point_t> points) {
    using namespace detail::split_point_cache;

    return std::make_unique<tree_container>(
        tree_container {tree_t {transform_view(points, to_tree_point)}});
}
}  // namespace

SplitPointCache::SplitPointCache(std::span<const point_t> points)
    : tree_ {make_tree(points)} {}

SplitPointCache::~SplitPointCache() = default;

SplitPointCache::SplitPointCache(SplitPointCache&&) = default;

auto SplitPointCache::operator=(SplitPointCache&&) -> SplitPointCache& = default;

auto SplitPointCache::add_split_point(point_t point) -> void {
    using namespace detail::split_point_cache;

    tree_->value.insert(to_tree_point(point));
}

auto SplitPointCache::query_is_inside(ordered_line_t line,
                                      std::vector<point_t>& result) const -> void {
    using namespace detail::split_point_cache;

    const auto inserter = [&result](const tree_point_t& point) {
        result.push_back(point_t {point.x(), point.y()});
    };

    result.clear();
    tree_->value.query(bgi::within(to_tree_line(line)), output_callable(inserter));
}

auto SplitPointCache::query_intersects(ordered_line_t line,
                                       std::vector<point_t>& result) const -> void {
    using namespace detail::split_point_cache;

    const auto inserter = [&result](const tree_point_t& point) {
        result.push_back(point_t {point.x(), point.y()});
    };

    result.clear();
    tree_->value.query(bgi::intersects(to_tree_line(line)), output_callable(inserter));
}

auto SplitPointCache::format() const -> std::string {
    return fmt::format("SplitPointCache = {}", tree_->value);
}

}  // namespace logicsim
