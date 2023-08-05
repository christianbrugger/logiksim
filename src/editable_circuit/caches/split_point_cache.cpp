#include "editable_circuit/caches/split_point_cache.h"

#include "layout.h"

namespace logicsim {

namespace detail::split_point_cache {

auto to_tree_point(point_t point) -> tree_point_t {
    return tree_point_t {point.x.value, point.y.value};
}

auto to_tree_line(ordered_line_t line) -> tree_line_t {
    return tree_line_t {to_tree_point(line.p0), to_tree_point(line.p1)};
}

}  // namespace detail::split_point_cache

SplitPointCache::SplitPointCache(std::span<const point_t> points)
    : tree_ {transform_view(points, detail::split_point_cache::to_tree_point)} {}

auto SplitPointCache::add_split_point(point_t point) -> void {
    using namespace detail::split_point_cache;

    tree_.insert(to_tree_point(point));
}

auto SplitPointCache::query_is_inside(ordered_line_t line,
                                      std::vector<point_t>& result) const -> void {
    using namespace detail::split_point_cache;

    const auto inserter = [&result](const tree_point_t& point) {
        result.push_back(point_t {point.x(), point.y()});
    };

    result.clear();
    tree_.query(bgi::within(to_tree_line(line)), output_callable(inserter));
}

auto SplitPointCache::query_intersects(ordered_line_t line,
                                       std::vector<point_t>& result) const -> void {
    using namespace detail::split_point_cache;

    const auto inserter = [&result](const tree_point_t& point) {
        result.push_back(point_t {point.x(), point.y()});
    };

    result.clear();
    tree_.query(bgi::intersects(to_tree_line(line)), output_callable(inserter));
}

auto SplitPointCache::format() const -> std::string {
    return fmt::format("SplitPointCache = {}", tree_);
}

}  // namespace logicsim
