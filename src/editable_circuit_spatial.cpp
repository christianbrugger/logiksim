#include "editable_circuit_spatial.h"

#include "circuit.h"
#include "format.h"
#include "iterator_adaptor.h"
#include "layout.h"
#include "layout_calculations.h"
#include "schematic.h"
#include "segment_tree.h"

#include <ankerl/unordered_dense.h>
#include <fmt/core.h>

#include <string>

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
        return logicsim::hash_8_byte(static_cast<uint32_t>(obj.element_id.value),
                                     static_cast<uint32_t>(obj.segment_index.value));
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

auto get_selection_box(line_t segment) -> tree_box_t {
    const auto rect = element_selection_rect(segment);
    return to_box(rect);
}

auto to_rect(tree_box_t box) -> rect_fine_t {
    const auto p0 = point_fine_t {box.min_corner().x(), box.min_corner().y()};
    const auto p1 = point_fine_t {box.max_corner().x(), box.max_corner().y()};

    return rect_fine_t {p0, p1};
}

auto to_box(rect_fine_t rect) -> tree_box_t {
    const auto p0 = tree_point_t {rect.p0.x, rect.p0.y};
    const auto p1 = tree_point_t {rect.p1.x, rect.p1.y};

    return tree_box_t {p0, p1};
}

}  // namespace detail::spatial_tree

auto SpatialTree::format() const -> std::string {
    return fmt::format("SpatialTree = {}\n", tree_);
}

auto SpatialTree::submit(editable_circuit::InfoMessage message) -> void {}

auto SpatialTree::insert(element_id_t element_id, layout_calculation_data_t data)
    -> void {
    if (is_placeholder(data)) {
        return;
    }
    if (data.element_type == ElementType::wire) {
        throw_exception("not implemented");
    }

    const auto box = detail::spatial_tree::get_selection_box(data);
    tree_.insert({box, {element_id, null_segment_index}});
}

auto SpatialTree::remove(element_id_t element_id, layout_calculation_data_t data)
    -> void {
    if (is_placeholder(data)) {
        return;
    }
    if (data.element_type == ElementType::wire) {
        throw_exception("not implemented");
    }

    const auto box = detail::spatial_tree::get_selection_box(data);
    const auto remove_count = tree_.remove({box, {element_id, null_segment_index}});

    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SpatialTree::update(element_id_t new_element_id, element_id_t old_element_id,
                         layout_calculation_data_t data) -> void {
    if (data.element_type == ElementType::wire) {
        for (auto i : range(data.segment_tree.segment_count())) {
            const auto segment = data.segment_tree.segment(i);
            const auto segment_index = gsl::narrow<segment_index_t::value_type>(i);

            // r-tree data is immutable
            remove(old_element_id, segment.line, segment_index_t {segment_index});
            insert(new_element_id, segment.line, segment_index_t {segment_index});
        }
    } else {
        // r-tree data is immutable
        remove(old_element_id, data);
        insert(new_element_id, data);
    }
}

auto SpatialTree::insert(element_id_t element_id, line_t segment, segment_index_t index)
    -> void {
    const auto box = detail::spatial_tree::get_selection_box(segment);
    tree_.insert({box, {element_id, index}});
}

auto SpatialTree::remove(element_id_t element_id, line_t segment, segment_index_t index)
    -> void {
    const auto box = detail::spatial_tree::get_selection_box(segment);

    const auto remove_count = tree_.remove({box, {element_id, index}});
    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SpatialTree::query_selection(rect_fine_t rect) const -> std::vector<query_result_t> {
    using namespace detail::spatial_tree;

    auto result = std::vector<query_result_t> {};

    const auto inserter
        = [&result](const tree_value_t& value) { result.push_back(value.second); };

    // intersects or covered_by
    tree_.query(bgi::intersects(to_box(rect)), output_callable(inserter));

    return result;
}

auto SpatialTree::query_line_segments(point_t grid_point) const -> queried_segments_t {
    using namespace detail::spatial_tree;

    const auto grid_point_fine = static_cast<point_fine_t>(grid_point);
    const auto tree_point = tree_point_t {grid_point_fine.x, grid_point_fine.y};

    auto result = std::array {null_segment, null_segment, null_segment, null_segment};

    const auto inserter
        = [&result, index = std::size_t {0}](const tree_value_t& value) mutable {
              if (value.second.segment_index == null_segment_index) {
                  // we only return segments
                  return;
              }
              result.at(index++)
                  = segment_t {value.second.element_id, value.second.segment_index};
          };

    tree_.query(bgi::intersects(tree_point), output_callable(inserter));
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

auto SpatialTree::validate(const Circuit& circuit) const -> void {
    using namespace detail::spatial_tree;

    auto cache = SpatialTree {};
    add_circuit_to_cache(cache, circuit);

    if (cache.tree_ != this->tree_) [[unlikely]] {
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

auto get_unique_element_id(SpatialTree::queried_segments_t result) -> element_id_t {
    const auto first_id = result.at(0).element_id;
    return (first_id && all_same_element_id(result)) ? first_id : null_element;
}

}  // namespace logicsim
