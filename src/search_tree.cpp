#include "search_tree.h"

#include "format.h"
#include "iterator_adaptor.h"
#include "layout_calculations.h"
#include "segment_tree.h"

#include <fmt/core.h>

#include <string>

namespace logicsim {

// Documentation:
// https://www.boost.org/doc/libs/1_81_0/libs/geometry/doc/html/geometry/spatial_indexes.html
//

//
// SearchTree
//

namespace detail::search_tree {

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

}  // namespace detail::search_tree

auto SearchTree::insert(element_id_t element_id, layout_calculation_data_t data) -> void {
    if (is_placeholder(data)) {
        return;
    }
    if (data.element_type == ElementType::wire) {
        throw_exception("not implemented");
    }

    const auto box = detail::search_tree::get_selection_box(data);
    tree_.insert({box, {element_id, null_segment_index}});
}

auto SearchTree::remove(element_id_t element_id, layout_calculation_data_t data) -> void {
    if (is_placeholder(data)) {
        return;
    }
    if (data.element_type == ElementType::wire) {
        throw_exception("not implemented");
    }

    const auto box = detail::search_tree::get_selection_box(data);
    const auto remove_count = tree_.remove({box, {element_id, null_segment_index}});

    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SearchTree::update(element_id_t new_element_id, element_id_t old_element_id,
                        layout_calculation_data_t data) -> void {
    if (data.element_type == ElementType::wire) {
        for (auto i : range(data.segment_tree.segment_count())) {
            const auto segment = data.segment_tree.segment(i);
            const auto segment_index = gsl::narrow<segment_index_t::value_type>(i);

            // r-tree data is immutable
            remove(old_element_id, segment.line, segment_index_t {segment_index});
            insert(new_element_id, segment.line, segment_index_t {segment_index});
        }
        for (auto &&segment : data.segment_tree.segments()) {
        }
    } else {
        // r-tree data is immutable
        remove(old_element_id, data);
        insert(new_element_id, data);
    }
}

auto SearchTree::insert(element_id_t element_id, line_t segment, segment_index_t index)
    -> void {
    const auto box = detail::search_tree::get_selection_box(segment);
    tree_.insert({box, {element_id, index}});
}

auto SearchTree::remove(element_id_t element_id, line_t segment, segment_index_t index)
    -> void {
    const auto box = detail::search_tree::get_selection_box(segment);

    const auto remove_count = tree_.remove({box, {element_id, index}});
    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SearchTree::query_selection(rect_fine_t rect) const -> std::vector<element_id_t> {
    using namespace detail::search_tree;

    auto result = std::vector<element_id_t> {};

    const auto inserter = [&result](const tree_value_t &value) {
        // TODO later add line segments
        if (value.second.segment_index == null_segment_index) {
            result.push_back(value.second.element_id);
        }
    };

    // intersects or covered_by
    tree_.query(bgi::intersects(to_box(rect)), output_callable(inserter));

    return result;
}

auto SearchTree::query_line_segments(point_t grid_point) const -> queried_segments_t {
    using namespace detail::search_tree;

    const auto grid_point_fine = static_cast<point_fine_t>(grid_point);
    const auto tree_point = tree_point_t {grid_point_fine.x, grid_point_fine.y};

    auto result = std::array {null_segment, null_segment, null_segment, null_segment};

    const auto inserter
        = [&result, index = std::size_t {0}](const tree_value_t &value) mutable {
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

auto get_unique_element_id(SearchTree::queried_segments_t result) -> element_id_t {
    const auto first_id = result.at(0).element_id;

    if (first_id == null_element) {
        return null_element;
    }

    const auto same_element_id
        = std::all_of(result.begin() + 1, result.end(), [=](segment_t value) {
              return value.element_id == null_element || value.element_id == first_id;
          });

    return same_element_id ? first_id : null_element;
}

}  // namespace logicsim
