#include "line_tree.h"

#include "algorithm/transform_to_container.h"
#include "allocated_size/trait.h"
#include "component/line_tree/tree_builder.h"
#include "geometry/orientation.h"
#include "line_tree.h"
#include "vocabulary/connection_count.h"

namespace logicsim {

LineTree::LineTree(line_tree::LineStore&& store__) : store_ {std::move(store__)} {}

auto LineTree::format() const -> std::string {
    return store_.format();
}

auto LineTree::empty() const noexcept -> bool {
    return store_.empty();
}

auto LineTree::size() const noexcept -> std::size_t {
    return store_.size();
}

auto LineTree::allocated_size() const -> std::size_t {
    return get_allocated_size(store_);
}

auto LineTree::begin() const -> iterator {
    return store_.lines().begin();
}

auto LineTree::end() const -> iterator {
    return store_.lines().end();
}

auto LineTree::lines() const -> std::span<const line_t> {
    return store_.lines();
}

auto LineTree::line(line_index_t index) const -> line_t {
    return store_.line(index);
}

auto LineTree::has_cross_point_p0(line_index_t index) const -> bool {
    return store_.starts_new_subtree(index);
}

auto LineTree::is_corner_p0(line_index_t index) const -> bool {
    return index > line_index_t {0} && !store_.starts_new_subtree(index);
}

auto LineTree::is_corner_p1(line_index_t index) const -> bool {
    if (index == store_.last_index()) {
        return false;
    }
    return !store_.starts_new_subtree(get_next(index));
}

auto LineTree::length_p0(line_index_t index) const -> length_t {
    return store_.start_length(index);
}

auto LineTree::length_p1(line_index_t index) const -> length_t {
    return store_.end_length(index);
}

auto LineTree::input_position() const -> point_t {
    return store_.line(line_index_t {0}).p0;
}

auto LineTree::input_orientation() const -> orientation_t {
    return to_orientation_p0(store_.line(line_index_t {0}));
}

auto LineTree::output_count() const -> connection_count_t {
    return connection_count_t {store_.leaf_indices().size()};
}

namespace {
auto get_leaf(const line_tree::LineStore& store, connection_id_t output) -> line_t {
    return store.line(store.leaf_indices().at(output.value));
}
}  // namespace

auto LineTree::output_position(connection_id_t output) const -> point_t {
    return get_leaf(store_, output).p1;
}

auto LineTree::output_orientation(connection_id_t output) const -> orientation_t {
    return to_orientation_p1(get_leaf(store_, output));
}

auto LineTree::calculate_output_lengths() const -> length_vector_t {
    static_assert(length_vector_t::max_size() >= line_tree::index_vector_t::max_size());

    return transform_to_container<length_vector_t>(
        store_.leaf_indices(),
        [&](const line_index_t& index) { return store_.end_length(index); });
};

//
// Public Functions
//

auto to_line_tree(std::span<const ordered_line_t> segments, point_t root) -> LineTree {
    return LineTree {line_tree::create_line_store(segments, root)};
}

auto indices(const LineTree& line_tree) -> range_extended_t<line_index_t> {
    return range_extended<line_index_t>(line_tree.size());
}

auto output_ids(const LineTree& line_tree) -> range_extended_t<connection_id_t> {
    return range_extended<connection_id_t>(std::size_t {line_tree.output_count()});
}

auto format_entry(const LineTree& line_tree, line_index_t index) -> std::string {
    const auto cross_p0 = line_tree.has_cross_point_p0(index);
    const auto corner_p0 = line_tree.is_corner_p0(index);
    const auto corner_p1 = line_tree.is_corner_p1(index);
    const auto line = line_tree.line(index);

    return fmt::format("({}{} {} - {} {})", cross_p0 ? "cross-point" : "",
                       corner_p0 ? "corner" : "", line.p0, line.p1,
                       corner_p1 ? "corner" : "");
}

}  // namespace logicsim
