#include "search_tree.h"

#include "format.h"
#include "layout_calculations.h"

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

auto get_selection_box(layout_calculation_data_t data) -> tree_box_t {
    const auto rect = static_cast<rect_fine_t>(element_collision_rect(data));

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
        return;
    }

    const auto box = detail::search_tree::get_selection_box(data);
    tree_.insert({box, element_id});
}

auto SearchTree::remove(element_id_t element_id, layout_calculation_data_t data) -> void {
    if (is_placeholder(data)) {
        return;
    }
    if (data.element_type == ElementType::wire) {
        return;
    }

    const auto box = detail::search_tree::get_selection_box(data);
    const auto remove_count = tree_.remove({box, element_id});

    if (remove_count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SearchTree::update(element_id_t new_element_id, element_id_t old_element_id,
                        layout_calculation_data_t data) -> void {
    // r-tree data is immutable
    remove(old_element_id, data);
    insert(new_element_id, data);
}

auto SearchTree::query_selection(rect_fine_t rect) const -> std::vector<element_id_t> {
    using namespace detail::search_tree;

    auto result = std::vector<element_id_t> {};

    // intersects or covered_by
    tree_.query(
        bgi::intersects(to_box(rect)),
        transform_output_iterator([](const tree_value_t &value) { return value.second; },
                                  std::back_inserter(result)));

    return result;
}

}  // namespace logicsim
