#include "search_tree.h"

#include "format.h"
#include "iterator_adaptor.h"
#include "layout.h"
#include "layout_calculations.h"
#include "schematic.h"
#include "segment_tree.h"

#include <ankerl/unordered_dense.h>
#include <fmt/core.h>

#include <string>

template <>
struct ankerl::unordered_dense::hash<logicsim::detail::search_tree::tree_payload_t> {
    using is_avalanching = void;
    using type = logicsim::detail::search_tree::tree_payload_t;

    [[nodiscard]] auto operator()(const type& obj) const noexcept -> uint64_t {
        return logicsim::hash_8_byte(static_cast<uint32_t>(obj.element_id.value),
                                     static_cast<uint32_t>(obj.segment_index.value));
    }
};

namespace logicsim {

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

auto SearchTree::query_selection(rect_fine_t rect) const -> std::vector<query_result_t> {
    using namespace detail::search_tree;

    auto result = std::vector<query_result_t> {};

    const auto inserter
        = [&result](const tree_value_t& value) { result.push_back(value.second); };

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

auto SearchTree::validate(const Layout& layout, const Schematic& schematic) const
    -> void {
    using namespace detail::search_tree;

    // collect all entries
    auto index = ankerl::unordered_dense::map<tree_payload_t, tree_box_t> {};
    for (auto&& item : tree_) {
        const auto [it, inserted] = index.try_emplace(item.second, item.first);
        if (!inserted) [[unlikely]] {
            throw_exception("found duplicate item in cache");
        }
    }

    // remove one-by-one
    const auto check_and_remove = [&index](tree_payload_t key, tree_box_t box) {
        const auto it = index.find(key);
        if (it == index.end()) [[unlikely]] {
            throw_exception("could not find item in index");
        }
        if (!bg::equals(it->second, box)) [[unlikely]] {
            throw_exception("cached box is different than the item");
        }
        index.erase(it);
    };

    for (const auto element : schematic.elements()) {
        const auto display_state = layout.display_state(element.element_id());
        // TODO reuse is_inserted ?
        const auto is_cached = display_state == display_state_t::new_valid
                               || display_state == display_state_t::normal;

        // elements
        if (element.is_element() && is_cached) {
            const auto key = tree_payload_t {.element_id = element.element_id()};
            const auto data
                = to_layout_calculation_data(schematic, layout, element.element_id());
            const auto box = get_selection_box(data);
            check_and_remove(key, box);
        }

        // line segments
        if (element.is_wire() && is_cached) {
            const auto& segment_tree = layout.segment_tree(element.element_id());

            for (const auto segment_index : segment_tree.indices()) {
                const auto key = tree_payload_t {.element_id = element.element_id(),
                                                 .segment_index = segment_index};
                const auto line = segment_tree.segment(segment_index).line;
                const auto box = get_selection_box(line);
                check_and_remove(key, box);
            }
        }
    }

    // leftover?
    if (!index.empty()) [[unlikely]] {
        throw_exception("found items in the index that don't exist anymore");
    }
}

auto get_segment_count(SearchTree::queried_segments_t result) -> int {
    return gsl::narrow_cast<int>(std::ranges::count_if(
        result, [](segment_t segment) { return bool {segment.element_id}; }));
}

auto all_same_element_id(SearchTree::queried_segments_t result) -> bool {
    const auto first_id = result.at(0).element_id;

    if (!first_id) {
        return true;
    }

    return std::all_of(result.begin() + 1, result.end(), [first_id](segment_t value) {
        return value.element_id == null_element || value.element_id == first_id;
    });
}

auto get_unique_element_id(SearchTree::queried_segments_t result) -> element_id_t {
    const auto first_id = result.at(0).element_id;
    return (first_id && all_same_element_id(result)) ? first_id : null_element;
}

}  // namespace logicsim
