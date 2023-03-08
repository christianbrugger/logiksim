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

auto get_box(layout_calculation_data_t data) -> tree_box_t {
    const auto rect = element_collision_rect(data);

    const auto min_corner = tree_point_t {rect.p0.x.value, rect.p0.y.value};
    const auto max_corner = tree_point_t {rect.p1.x.value, rect.p1.y.value};

    return tree_box_t {min_corner, max_corner};
}

}  // namespace detail::search_tree

auto SearchTree::insert(element_id_t element_id, layout_calculation_data_t data) -> void {
    const auto box = detail::search_tree::get_box(data);
    tree_.insert({box, element_id});
}

auto SearchTree::remove(element_id_t element_id, layout_calculation_data_t data) -> void {
    const auto box = detail::search_tree::get_box(data);
    const auto count = tree_.remove({box, element_id});

    if (count != 1) [[unlikely]] {
        throw_exception("Wasn't able to find element to remove.");
    }
}

auto SearchTree::update(element_id_t new_element_id, element_id_t old_element_id,
                        layout_calculation_data_t data) -> void {
    // r-tree data is immutable
    remove(old_element_id, data);
    insert(new_element_id, data);
}

}  // namespace logicsim
