#include "editable_circuit/cache/spatial_cache.h"

#include "editable_circuit/cache/helper.h"
#include "editable_circuit/message.h"
#include "layout.h"
#include "layout_calculation.h"
#include "wyhash.h"

#include <boost/geometry.hpp>

namespace logicsim::detail::spatial_tree {
// Boost R-Tree Documentation:
// https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/spatial_indexes.html

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using tree_point_t = bg::model::d2::point_xy<grid_fine_t::value_type>;
using tree_box_t = bg::model::box<tree_point_t>;
using tree_value_t = std::pair<tree_box_t, tree_payload_t>;

static_assert(sizeof(tree_box_t) == 32);
static_assert(sizeof(tree_value_t) == 40);

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<tree_value_t, bgi::rstar<tree_max_node_elements>>;

auto get_selection_box(layout_calculation_data_t data) -> tree_box_t;
auto get_selection_box(ordered_line_t segment) -> tree_box_t;
auto to_tree_point(point_fine_t point) -> tree_point_t;
auto to_rect(tree_box_t box) -> rect_fine_t;
auto to_box(rect_fine_t rect) -> tree_box_t;

auto operator==(const tree_t& a, const tree_t& b) -> bool;
auto operator!=(const tree_t& a, const tree_t& b) -> bool;

struct tree_container {
    tree_t value;
};

}  // namespace logicsim::detail::spatial_tree

namespace boost::geometry::model {
template <typename T>
auto operator==(box<T> a, box<T> b) -> bool {
    return equals(a, b);
}
}  // namespace boost::geometry::model

template <>
struct ankerl::unordered_dense::hash<logicsim::detail::spatial_tree::tree_payload_t> {
    using is_avalanching = void;
    using type = logicsim::detail::spatial_tree::tree_payload_t;

    [[nodiscard]] auto operator()(const type& obj) const noexcept -> uint64_t {
        static_assert(std::is_same_v<int32_t, logicsim::element_id_t::value_type>);
        static_assert(std::is_same_v<int32_t, logicsim::segment_index_t::value_type>);

        return logicsim::wyhash_64_bit(obj.element_id.value, obj.segment_index.value);
    }
};

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::detail::spatial_tree::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::spatial_tree::tree_point_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

template <>
struct fmt::formatter<logicsim::detail::spatial_tree::tree_box_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::spatial_tree::tree_box_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.min_corner(), obj.max_corner());
    }
};

template <>
struct fmt::formatter<logicsim::detail::spatial_tree::tree_value_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::detail::spatial_tree::tree_value_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}: {}", obj.first, obj.second);
    }
};

namespace logicsim {

//
// SpatialTree
//

namespace detail::spatial_tree {

auto tree_payload_t::format() const -> std::string {
    return fmt::format("<Element {}, Segment {}>", element_id, segment_index);
}

auto get_selection_box(layout_calculation_data_t data) -> tree_box_t {
    const auto rect = element_selection_rect(data);
    return to_box(rect);
}

auto get_selection_box(ordered_line_t line) -> tree_box_t {
    const auto rect = element_selection_rect(line);
    return to_box(rect);
}

auto to_tree_point(point_fine_t point) -> tree_point_t {
    return tree_point_t {double {point.x}, double {point.y}};
}

auto to_rect(tree_box_t box) -> rect_fine_t {
    const auto p0 = point_fine_t {box.min_corner().x(), box.min_corner().y()};
    const auto p1 = point_fine_t {box.max_corner().x(), box.max_corner().y()};

    return rect_fine_t {p0, p1};
}

auto to_box(rect_fine_t rect) -> tree_box_t {
    const auto p0 = to_tree_point(rect.p0);
    const auto p1 = to_tree_point(rect.p1);

    return tree_box_t {p0, p1};
}

}  // namespace detail::spatial_tree

SpatialTree::SpatialTree()
    : tree_ {std::make_unique<detail::spatial_tree::tree_container>()} {}

SpatialTree::~SpatialTree() = default;

SpatialTree::SpatialTree(SpatialTree&&) = default;

auto SpatialTree::operator=(SpatialTree&&) -> SpatialTree& = default;

auto SpatialTree::format() const -> std::string {
    auto it = tree_->value.begin();
    return fmt::format("SpatialTree = {}", tree_->value);
}

auto SpatialTree::handle(const editable_circuit::info_message::LogicItemInserted& message)
    -> void {
    const auto box = detail::spatial_tree::get_selection_box(message.data);
    tree_->value.insert({box, {message.element_id, null_segment_index}});
}

auto SpatialTree::handle(
    const editable_circuit::info_message::LogicItemUninserted& message) -> void {
    const auto box = detail::spatial_tree::get_selection_box(message.data);
    const auto remove_count =
        tree_->value.remove({box, {message.element_id, null_segment_index}});

    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SpatialTree::handle(
    const editable_circuit::info_message::InsertedLogicItemIdUpdated& message) -> void {
    using namespace editable_circuit::info_message;

    // r-tree data is immutable
    handle(LogicItemUninserted {message.old_element_id, message.data});
    handle(LogicItemInserted {message.new_element_id, message.data});
}

auto SpatialTree::handle(const editable_circuit::info_message::SegmentInserted& message)
    -> void {
    const auto box = detail::spatial_tree::get_selection_box(message.segment_info.line);
    tree_->value.insert(
        {box, {message.segment.element_id, message.segment.segment_index}});
}

auto SpatialTree::handle(const editable_circuit::info_message::SegmentUninserted& message)
    -> void {
    const auto box = detail::spatial_tree::get_selection_box(message.segment_info.line);

    const auto remove_count = tree_->value.remove(
        {box, {message.segment.element_id, message.segment.segment_index}});

    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SpatialTree::handle(
    const editable_circuit::info_message::InsertedSegmentIdUpdated& message) -> void {
    using namespace editable_circuit::info_message;

    // r-tree data is immutable
    handle(SegmentUninserted {message.old_segment, message.segment_info});
    handle(SegmentInserted {message.new_segment, message.segment_info});
}

auto SpatialTree::submit(const editable_circuit::InfoMessage& message) -> void {
    using namespace editable_circuit::info_message;

    // logic items
    if (auto pointer = std::get_if<LogicItemInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemUninserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedLogicItemIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }

    // segments
    if (auto pointer = std::get_if<SegmentInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<SegmentUninserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedSegmentIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
}

auto SpatialTree::query_selection(rect_fine_t rect) const -> std::vector<query_result_t> {
    using namespace detail::spatial_tree;

    auto result = std::vector<query_result_t> {};

    const auto inserter = [&result](const tree_value_t& value) {
        result.push_back(value.second);
    };

    // intersects or covered_by
    tree_->value.query(bgi::intersects(to_box(rect)), output_callable(inserter));

    return result;
}

auto SpatialTree::has_element(point_fine_t point) const -> bool {
    using namespace detail::spatial_tree;

    return tree_->value.qbegin(bgi::intersects(to_tree_point(point))) !=
           tree_->value.qend();
}

auto SpatialTree::query_line_segments(point_t grid_point) const -> queried_segments_t {
    using namespace detail::spatial_tree;

    const auto tree_point = to_tree_point(point_fine_t {grid_point});

    auto result = std::array {null_segment, null_segment, null_segment, null_segment};

    const auto inserter = [&result,
                           index = std::size_t {0}](const tree_value_t& value) mutable {
        if (value.second.segment_index == null_segment_index) {
            // we only return segments
            return;
        }
        result.at(index++) =
            segment_t {value.second.element_id, value.second.segment_index};
    };

    tree_->value.query(bgi::intersects(tree_point), output_callable(inserter));
    return result;
}

auto SpatialTree::rects() const -> std::vector<rect_fine_t> {
    auto result = std::vector<rect_fine_t> {};
    result.reserve(tree_->value.size());

    for (auto& value : tree_->value) {
        result.push_back(detail::spatial_tree::to_rect(value.first));
    }

    return result;
}

namespace detail::spatial_tree {

using index_map_t = ankerl::unordered_dense::map<tree_payload_t, tree_box_t>;

auto to_reverse_index(const tree_t& tree) -> index_map_t {
    auto index = index_map_t {};

    for (auto&& item : tree) {
        const auto inserted = index.try_emplace(item.second, item.first).second;
        if (!inserted) [[unlikely]] {
            throw_exception("found duplicate item in cache");
        }
    }

    return index;
}

auto operator==(const tree_t& a, const tree_t& b) -> bool {
    const auto index_a = to_reverse_index(a);
    const auto index_b = to_reverse_index(b);
    return index_a == index_b;
}

auto operator!=(const tree_t& a, const tree_t& b) -> bool {
    return !(a == b);
}

}  // namespace detail::spatial_tree

auto SpatialTree::validate(const Layout& layout) const -> void {
    using namespace detail::spatial_tree;

    auto cache = SpatialTree {};
    add_layout_to_cache(cache, layout);

    if (cache.tree_->value != this->tree_->value) [[unlikely]] {
        print(layout);
        print("expected state =", cache);
        print("actual state   =", *this);
        throw_exception("current cache state doesn't match circuit");
    }
}

auto get_segment_count(SpatialTree::queried_segments_t result) -> int {
    return gsl::narrow_cast<int>(std::ranges::count_if(
        result, [](segment_t segment) { return bool {segment.element_id}; }));
}

auto all_same_element_id(SpatialTree::queried_segments_t result) -> bool {
    const auto first_id = result.at(0).element_id;

    if (!first_id) {
        return true;
    }

    return std::all_of(result.begin() + 1, result.end(), [first_id](segment_t value) {
        return value.element_id == null_element || value.element_id == first_id;
    });
}

auto get_segment_indices(SpatialTree::queried_segments_t result)
    -> std::array<segment_index_t, 4> {
    static_assert(result.size() == 4);
    return std::array {
        result.at(0).segment_index,
        result.at(1).segment_index,
        result.at(2).segment_index,
        result.at(3).segment_index,
    };
}

auto get_unique_element_id(SpatialTree::queried_segments_t result) -> element_id_t {
    if (!result.at(0).element_id) {
        throw_exception("result has not segments");
    }
    if (!all_same_element_id(result)) {
        throw_exception("result has different ids");
    }
    return result.at(0).element_id;
}

}  // namespace logicsim
