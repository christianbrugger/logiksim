#include "core/index/spatial_index.h"

#include "core/algorithm/accumulate.h"
#include "core/allocated_size/tracked_resource.h"
#include "core/concept/input_range.h"
#include "core/format/container.h"
#include "core/iterator_adaptor/output_callable.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/layout_message.h"
#include "core/layout_message_generation.h"
#include "core/selection.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/rect_fine.h"
#include "core/wyhash.h"

#include <boost/geometry.hpp>

#include <algorithm>
#include <memory_resource>

namespace logicsim::spatial_index {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using tree_coordinate_type = grid_fine_t::value_type;
using tree_point_t = bg::model::d2::point_xy<tree_coordinate_type>;
using tree_box_t = bg::model::box<tree_point_t>;
using tree_value_t = std::pair<tree_box_t, tree_payload_t>;

static_assert(sizeof(tree_box_t) == 32);
static_assert(sizeof(tree_value_t) == 40);

constexpr inline static auto tree_max_node_elements = 16;
using tree_t = bgi::rtree<                         //
    tree_value_t,                                  // Value
    bgi::rstar<tree_max_node_elements>,            // Parameters
    bgi::indexable<tree_value_t>,                  // IndexableGetter
    bgi::equal_to<tree_value_t>,                   // EqualTo
    std::pmr::polymorphic_allocator<tree_value_t>  // Allocator
    >;

[[nodiscard]] auto get_selection_box(const layout_calculation_data_t& data) -> tree_box_t;
[[nodiscard]] auto get_selection_box(ordered_line_t line) -> tree_box_t;
[[nodiscard]] auto to_tree_point(point_fine_t point) -> tree_point_t;
[[nodiscard]] auto to_rect(tree_box_t box) -> rect_fine_t;
[[nodiscard]] auto to_box(rect_fine_t rect) -> tree_box_t;
[[nodiscard]] auto get_all_tree_values(const Layout& layout) -> std::vector<tree_value_t>;

[[nodiscard]] auto operator==(const tree_t& a, const tree_t& b) -> bool;
[[nodiscard]] auto operator!=(const tree_t& a, const tree_t& b) -> bool;

struct tree_container {
    tracked_resource resource;
    tree_t value;

    explicit tree_container();
    tree_container(const tree_container& other);
    ~tree_container() = default;

    template <input_range_of<tree_value_t> R>
    tree_container(const R& range);

    // delete as resource is referenced, and its not needed by value_pointer
    tree_container(tree_container&& other) noexcept = delete;
    auto operator=(const tree_container&) -> tree_container& = delete;
    auto operator=(tree_container&&) noexcept -> tree_container& = delete;

    [[nodiscard]] auto operator==(const tree_container& other) const -> bool;
};

tree_container::tree_container()
    : resource {},
      value {tree_t {}, std::pmr::polymorphic_allocator<tree_value_t> {&resource}} {}

tree_container::tree_container(const tree_container& other)
    : resource {},
      value {other.value, std::pmr::polymorphic_allocator<tree_value_t> {&resource}} {}

template <input_range_of<tree_value_t> R>
tree_container::tree_container(const R& range)
    : resource {},
      value {tree_t {std::ranges::begin(range), std::ranges::end(range)},
             std::pmr::polymorphic_allocator<tree_value_t> {&resource}} {}

auto tree_container::operator==(const tree_container& other) const -> bool {
    return value == other.value;
}

}  // namespace logicsim::spatial_index

namespace boost::geometry::model {
template <typename T>
auto operator==(box<T> a, box<T> b) -> bool {
    return equals(a, b);
}

}  // namespace boost::geometry::model

template <>
struct ankerl::unordered_dense::hash<logicsim::spatial_index::tree_payload_t> {
    using is_avalanching = void;
    using type = logicsim::spatial_index::tree_payload_t;

    [[nodiscard]] auto operator()(const type& obj) const noexcept -> uint64_t {
        return obj.hash();
    }
};

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::spatial_index::tree_point_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::spatial_index::tree_point_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.x(), obj.y());
    }
};

template <>
struct fmt::formatter<logicsim::spatial_index::tree_box_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::spatial_index::tree_box_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", obj.min_corner(), obj.max_corner());
    }
};

template <>
struct fmt::formatter<logicsim::spatial_index::tree_value_t> {
    static constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::spatial_index::tree_value_t& obj,
                       fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}: {}", obj.first, obj.second);
    }
};

namespace logicsim {

//
// SpatialIndex
//

namespace spatial_index {

auto tree_payload_t::format() const -> std::string {
    if (is_logicitem()) {
        return fmt::format("<LogicItem {}>", logicitem());
    }
    if (is_segment()) {
        return fmt::format("<Segment {}>", segment());
    }
    if (is_decoration()) {
        return fmt::format("<Decoration {}>", decoration());
    }
    std::terminate();
}

auto tree_payload_t::hash() const -> uint64_t {
    static_assert(std::is_same_v<int32_t, decltype(element_id_)>);
    static_assert(std::is_same_v<int32_t, decltype(segment_index_.value)>);

    return logicsim::wyhash_64_bit(element_id_, segment_index_.value);
}

tree_payload_t::tree_payload_t(logicitem_id_t logicitem_id)
    : element_id_ {logicitem_id.value}, segment_index_ {logicitem_tag} {
    static_assert(std::is_same_v<logicitem_id_t::value_type, decltype(element_id_)>);

    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("logicitem id cannot be null");
    }
}

tree_payload_t::tree_payload_t(decoration_id_t decoration_id)
    : element_id_ {decoration_id.value}, segment_index_ {decoration_tag} {
    static_assert(std::is_same_v<decoration_id_t::value_type, decltype(element_id_)>);

    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("decoration id cannot be null");
    }
}

tree_payload_t::tree_payload_t(segment_t segment)
    : element_id_ {segment.wire_id.value}, segment_index_ {segment.segment_index} {
    static_assert(std::is_same_v<wire_id_t::value_type, decltype(element_id_)>);

    if (!segment.wire_id || !segment_index_) [[unlikely]] {
        throw std::runtime_error("segment cannot be null");
    }
}

auto tree_payload_t::is_logicitem() const -> bool {
    return segment_index_ == logicitem_tag;
}

auto tree_payload_t::logicitem() const -> logicitem_id_t {
    Expects(is_logicitem());

    static_assert(std::is_same_v<logicitem_id_t::value_type, decltype(element_id_)>);
    return logicitem_id_t {element_id_};
}

auto tree_payload_t::is_decoration() const -> bool {
    return segment_index_ == decoration_tag;
}

auto tree_payload_t::decoration() const -> decoration_id_t {
    Expects(is_decoration());

    static_assert(std::is_same_v<decoration_id_t::value_type, decltype(element_id_)>);
    return decoration_id_t {element_id_};
}

auto tree_payload_t::is_segment() const -> bool {
    return bool {segment_index_};
}

auto tree_payload_t::segment() const -> segment_t {
    Expects(is_segment());

    static_assert(std::is_same_v<wire_id_t::value_type, decltype(element_id_)>);
    return segment_t {wire_id_t {element_id_}, segment_index_};
}

auto get_selection_box(const layout_calculation_data_t& data) -> tree_box_t {
    const auto rect = element_selection_rect(data);
    return to_box(rect);
}

auto get_selection_box(const decoration_layout_data_t& data) -> tree_box_t {
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

auto get_all_tree_values(const Layout& layout) -> std::vector<tree_value_t> {
    const auto count = get_inserted_logicitem_count(layout) +
                       get_inserted_decoration_count(layout) +
                       get_inserted_segment_count(layout);
    auto values = std::vector<tree_value_t> {};
    values.reserve(count);

    for (const auto& logicitem_id : logicitem_ids(layout)) {
        if (is_inserted(layout, logicitem_id)) {
            const auto data = to_layout_calculation_data(layout, logicitem_id);
            const auto box = get_selection_box(data);
            values.emplace_back(box, tree_payload_t {logicitem_id});
        }
    }

    for (const auto& decoration_id : decoration_ids(layout)) {
        if (is_inserted(layout, decoration_id)) {
            const auto data = to_decoration_layout_data(layout, decoration_id);
            const auto box = get_selection_box(data);
            values.emplace_back(box, tree_payload_t {decoration_id});
        }
    }

    for (const auto& wire_id : inserted_wire_ids(layout)) {
        const auto& tree = layout.wires().segment_tree(wire_id);
        for (const auto& segment_index : tree.indices()) {
            const auto box = get_selection_box(tree.line(segment_index));
            values.emplace_back(box, tree_payload_t {segment_t {wire_id, segment_index}});
        }
    }

    Expects(values.size() == count);
    return values;
}

}  // namespace spatial_index

template class value_pointer<spatial_index::tree_container, equality_comparable>;

SpatialIndex::SpatialIndex(const Layout& layout)
    // Using RTree bulk insertion is 6x faster than generate_layout_messages
    : tree_ {spatial_index::get_all_tree_values(layout)} {}

auto SpatialIndex::format() const -> std::string {
    return fmt::format("SpatialIndex = {}", tree_->value);
}

auto SpatialIndex::allocated_size() const -> std::size_t {
    return tree_->resource.allocated_size();
}

//
// LogicItem
//

auto SpatialIndex::handle(const info_message::LogicItemInserted& message) -> void {
    const auto box = spatial_index::get_selection_box(message.data);
    tree_->value.insert({box, value_t {message.logicitem_id}});
}

auto SpatialIndex::handle(const info_message::LogicItemUninserted& message) -> void {
    const auto box = spatial_index::get_selection_box(message.data);
    const auto remove_count = tree_->value.remove({box, value_t {message.logicitem_id}});

    if (remove_count != 1) [[unlikely]] {
        throw std::runtime_error("Not able to find element to remove.");
    }
}

auto SpatialIndex::handle(const info_message::InsertedLogicItemIdUpdated& message)
    -> void {
    using namespace info_message;

    // r-tree data is immutable
    handle(LogicItemUninserted {message.old_logicitem_id, message.data});
    handle(LogicItemInserted {message.new_logicitem_id, message.data});
}

//
// Decoration
//

auto SpatialIndex::handle(const info_message::DecorationInserted& message) -> void {
    const auto box = spatial_index::get_selection_box(message.data);
    tree_->value.insert({box, value_t {message.decoration_id}});
}

auto SpatialIndex::handle(const info_message::DecorationUninserted& message) -> void {
    const auto box = spatial_index::get_selection_box(message.data);
    const auto remove_count = tree_->value.remove({box, value_t {message.decoration_id}});

    if (remove_count != 1) [[unlikely]] {
        throw std::runtime_error("Not able to find element to remove.");
    }
}

auto SpatialIndex::handle(const info_message::InsertedDecorationIdUpdated& message)
    -> void {
    using namespace info_message;

    // r-tree data is immutable
    handle(DecorationUninserted {message.old_decoration_id, message.data});
    handle(DecorationInserted {message.new_decoration_id, message.data});
}

//
// Wire Segment
//

auto SpatialIndex::handle(const info_message::SegmentInserted& message) -> void {
    const auto box = spatial_index::get_selection_box(message.segment_info.line);
    tree_->value.insert({box, value_t {message.segment}});
}

auto SpatialIndex::handle(const info_message::SegmentUninserted& message) -> void {
    const auto box = spatial_index::get_selection_box(message.segment_info.line);
    const auto remove_count = tree_->value.remove({box, value_t {message.segment}});

    if (remove_count != 1) [[unlikely]] {
        throw std::runtime_error("Not able to find element to remove.");
    }
}

auto SpatialIndex::handle(const info_message::InsertedSegmentIdUpdated& message) -> void {
    using namespace info_message;

    // Deletion and re-insertion is not a performance problem: when un-inserting 500k line
    // segments 1975 ms (this approach) vs 1927 ms (using hacky query & const_cast)
    // overall performance.

    // r-tree data is immutable
    handle(SegmentUninserted {message.old_segment, message.segment_info});
    handle(SegmentInserted {message.new_segment, message.segment_info});
}

auto SpatialIndex::submit(const InfoMessage& message) -> void {
    using namespace info_message;

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

    // decorations
    if (auto pointer = std::get_if<DecorationInserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<DecorationUninserted>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<InsertedDecorationIdUpdated>(&message)) {
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

auto SpatialIndex::query_selection(rect_fine_t rect) const -> std::vector<value_t> {
    using namespace spatial_index;
    auto result = std::vector<value_t> {};

    std::ranges::transform(tree_->value.qbegin(bgi::intersects(to_box(rect))),
                           tree_->value.qend(), std::back_inserter(result),
                           &tree_value_t::second);
    return result;
}

auto SpatialIndex::has_element(point_fine_t point) const -> bool {
    using namespace spatial_index;

    return tree_->value.qbegin(bgi::intersects(to_tree_point(point))) !=
           tree_->value.qend();
}

auto SpatialIndex::query_line_segments(point_t grid_point) const -> queried_segments_t {
    using namespace spatial_index;

    const auto tree_point = to_tree_point(point_fine_t {grid_point});

    auto result = std::array {null_segment, null_segment, null_segment, null_segment};

    const auto inserter = [&result,
                           index = std::size_t {0}](const tree_value_t& value) mutable {
        if (value.second.is_segment()) {
            result.at(index++) = value.second.segment();
        }
    };

    tree_->value.query(bgi::intersects(tree_point), output_callable(inserter));
    return result;
}

auto SpatialIndex::rects() const -> std::vector<rect_fine_t> {
    auto result = std::vector<rect_fine_t> {};
    result.reserve(tree_->value.size());

    for (const auto& value : tree_->value) {
        result.push_back(spatial_index::to_rect(value.first));
    }

    return result;
}

namespace spatial_index {

using index_map_t = ankerl::unordered_dense::map<tree_payload_t, tree_box_t>;

auto to_reverse_index(const tree_t& tree) -> index_map_t {
    auto index = index_map_t {};
    index.reserve(tree.size());

    for (auto&& item : tree) {
        const bool inserted = index.try_emplace(item.second, item.first).second;
        if (!inserted) [[unlikely]] {
            throw std::runtime_error("found duplicate item in index");
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

}  // namespace spatial_index

auto get_segment_count(SpatialIndex::queried_segments_t result) -> int {
    return gsl::narrow_cast<int>(std::ranges::count_if(
        result, [](segment_t segment) { return bool {segment.wire_id}; }));
}

auto all_same_wire_id(SpatialIndex::queried_segments_t result) -> bool {
    const auto first_id = result.at(0).wire_id;

    if (!first_id) {
        return true;
    }

    return std::all_of(result.begin() + 1, result.end(), [first_id](segment_t value) {
        return value.wire_id == null_wire_id || value.wire_id == first_id;
    });
}

auto get_segment_indices(SpatialIndex::queried_segments_t result)
    -> std::array<segment_index_t, 4> {
    static_assert(result.size() == 4);
    return std::array {
        result.at(0).segment_index,
        result.at(1).segment_index,
        result.at(2).segment_index,
        result.at(3).segment_index,
    };
}

auto get_unique_wire_id(SpatialIndex::queried_segments_t result) -> wire_id_t {
    if (!result.at(0).wire_id) {
        throw std::runtime_error("result has not segments");
    }
    if (!all_same_wire_id(result)) {
        throw std::runtime_error("result has different ids");
    }
    return result.at(0).wire_id;
}

auto is_selected(const SpatialIndex::value_t& item, point_fine_t point,
                 const Selection& selection, const Layout& layout) -> bool {
    return (item.is_logicitem() && selection.is_selected(item.logicitem())) ||
           (item.is_segment() && is_selected(selection, layout, item.segment(), point)) ||
           (item.is_decoration() && selection.is_selected(item.decoration()));
}

auto anything_selected(std::span<const SpatialIndex::value_t> items, point_fine_t point,
                       const Selection& selection, const Layout& layout) -> bool {
    const auto index_selected = [&](const SpatialIndex::value_t& item) {
        return is_selected(item, point, selection, layout);
    };

    return std::ranges::any_of(items, index_selected);
}

auto all_selected(std::span<const SpatialIndex::value_t> items, point_fine_t point,
                  const Selection& selection, const Layout& layout) -> bool {
    const auto index_selected = [&](const SpatialIndex::value_t& item) {
        return is_selected(item, point, selection, layout);
    };

    return std::ranges::all_of(items, index_selected);
}

}  // namespace logicsim
