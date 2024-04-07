#include "index/spatial_point_index.h"

#include "format/container.h"
#include "vocabulary/ordered_line.h"

#include <boost/geometry.hpp>

namespace logicsim::spatial_point_index {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using tree_point_t = bg::model::d2::point_xy<grid_t::value_type>;
using tree_line_t = bg::model::segment<tree_point_t>;
static_assert(sizeof(tree_point_t) == 4);
static_assert(sizeof(tree_line_t) == 8);

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<tree_point_t, bgi::rstar<tree_max_node_elements>>;

[[nodiscard]] auto to_tree_point(point_t point) -> tree_point_t;
[[nodiscard]] auto to_tree_line(ordered_line_t line) -> tree_line_t;
[[nodiscard]] auto from_tree_point(const tree_point_t& point) -> point_t;

struct tree_container {
    tree_t value;

    // [[nodiscard]] auto operator==(const tree_container& other) const -> bool = default;
};
}  // namespace logicsim::spatial_point_index

template <>
struct fmt::formatter<logicsim::spatial_point_index::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::spatial_point_index::tree_point_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

namespace logicsim {

namespace spatial_point_index {

auto to_tree_point(point_t point) -> tree_point_t {
    return tree_point_t {point.x.value, point.y.value};
}

auto to_tree_line(ordered_line_t line) -> tree_line_t {
    return tree_line_t {to_tree_point(line.p0), to_tree_point(line.p1)};
}

auto from_tree_point(const tree_point_t& point) -> point_t {
    return point_t {point.x(), point.y()};
};

}  // namespace spatial_point_index

SpatialPointIndex::SpatialPointIndex()
    : tree_ {std::make_unique<spatial_point_index::tree_container>()} {}

namespace {
auto make_tree(std::span<const point_t> points) {
    using namespace spatial_point_index;

    return std::make_unique<tree_container>(
        tree_container {tree_t {transform_view(points, to_tree_point)}});
}
}  // namespace

SpatialPointIndex::SpatialPointIndex(std::span<const point_t> points)
    : tree_ {make_tree(points)} {}

SpatialPointIndex::~SpatialPointIndex() = default;

SpatialPointIndex::SpatialPointIndex(const SpatialPointIndex& other)
    : tree_ {std::make_unique<spatial_point_index::tree_container>(*other.tree_)} {}

auto SpatialPointIndex::operator=(const SpatialPointIndex& other) -> SpatialPointIndex& {
    auto tmp = SpatialPointIndex {other};
    using std::swap;
    swap(*this, tmp);
    return *this;
}

SpatialPointIndex::SpatialPointIndex(SpatialPointIndex&&) = default;

auto SpatialPointIndex::operator=(SpatialPointIndex&&) -> SpatialPointIndex& = default;

auto SpatialPointIndex::add_split_point(point_t point) -> void {
    using namespace spatial_point_index;

    tree_->value.insert(to_tree_point(point));
}

auto SpatialPointIndex::query_is_inside(ordered_line_t line) const -> point_vector_t {
    using namespace spatial_point_index;
    auto result = point_vector_t {};

    std::ranges::transform(tree_->value.qbegin(bgi::within(to_tree_line(line))),
                           tree_->value.qend(), std::back_inserter(result),
                           from_tree_point);

    return result;
}

auto SpatialPointIndex::query_intersects(ordered_line_t line) const -> point_vector_t {
    using namespace spatial_point_index;
    auto result = point_vector_t {};

    std::ranges::transform(tree_->value.qbegin(bgi::intersects(to_tree_line(line))),
                           tree_->value.qend(), std::back_inserter(result),
                           from_tree_point);

    return result;
}

auto SpatialPointIndex::format() const -> std::string {
    return fmt::format("SpatialPointIndex = {}", tree_->value);
}

}  // namespace logicsim
