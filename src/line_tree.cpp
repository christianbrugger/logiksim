

#include "line_tree.h"

#include "algorithm.h"
#include "collision.h"
#include "exceptions.h"
#include "format.h"
#include "graph.h"
#include "range.h"

#include <boost/container/small_vector.hpp>
#include <gsl/gsl>

#include <algorithm>
#include <array>
#include <functional>
#include <numeric>
#include <optional>
#include <utility>

namespace logicsim {

//
// merging
//

class SegmentSplitter {
   public:
    using buffer_t = std::vector<line2d_t>;

    SegmentSplitter() {
        buffer_.reserve(16);
    }

    auto split_segment(line2d_t segment, std::ranges::input_range auto&& points)
        -> const buffer_t& {
        buffer_.clear();
        buffer_.push_back(segment);

        for (auto point : points) {
            auto splittable = std::ranges::find_if(
                buffer_,
                [&point](line2d_t line) -> bool { return is_inside(point, line); });

            if (splittable != buffer_.end()) {
                buffer_.push_back(line2d_t {point, splittable->p1});
                *splittable = line2d_t {splittable->p0, point};
            }
        }

        return buffer_;
    }

   private:
    buffer_t buffer_ {};
};

auto split_segments(std::ranges::input_range auto&& segments,
                    std::ranges::input_range auto&& points) -> std::vector<line2d_t> {
    std::vector<line2d_t> result;
    result.reserve(std::size(segments) + std::size(points));

    auto splitter = SegmentSplitter {};
    for (auto segment : segments) {
        std::ranges::copy(splitter.split_segment(segment, points),
                          std::back_inserter(result));
    }
    return result;
}

template <class OutputIterator, class GetterSame, class GetterDifferent>
auto merge_segments_1d(const line_tree_vector_t& line_trees, OutputIterator result,
                       GetterSame get_same, GetterDifferent get_different) -> void {
    // collect lines
    auto parallel_segments = std::vector<line2d_t> {};
    for (auto tree_reference : line_trees) {
        transform_if(
            tree_reference.get().segments(), std::back_inserter(parallel_segments),
            [&](auto line) -> line2d_t { return order_points(line); },
            [&](auto line) -> bool { return get_same(line.p0) == get_same(line.p1); });
    }

    // sort lists
    std::ranges::sort(parallel_segments, [&](line2d_t a, line2d_t b) {
        return std::tie(get_same(a.p0), get_different(a.p0))
               < std::tie(get_same(b.p0), get_different(b.p0));
    });

    // extract elements
    transform_combine_while(
        parallel_segments, result,
        // make state
        [](auto it) -> line2d_t { return *it; },
        // combine while
        [&](line2d_t state, auto it) -> bool {
            return get_same(state.p0) == get_same(it->p0)
                   && get_different(state.p1) >= get_different(it->p0);
        },
        // update state
        [&](line2d_t state, auto it) -> line2d_t {
            get_different(state.p1)
                = std::max(get_different(state.p1), get_different(it->p1));
            return state;
        });
}

auto sum_segment_counts(const line_tree_vector_t& line_trees) -> size_t {
    return std::transform_reduce(
        line_trees.begin(), line_trees.end(), size_t {0}, std::plus<>(),
        [](auto tree_reference) { return tree_reference.get().segment_count(); });
}

auto merge_segments(const line_tree_vector_t& line_trees) -> std::vector<line2d_t> {
    auto result = std::vector<line2d_t> {};
    result.reserve(sum_segment_counts(line_trees));

    auto get_x = [](point2d_t& point) -> grid_t& { return point.x; };
    auto get_y = [](point2d_t& point) -> grid_t& { return point.y; };

    // vertical & horizontal
    merge_segments_1d(line_trees, std::back_inserter(result), get_x, get_y);
    merge_segments_1d(line_trees, std::back_inserter(result), get_y, get_x);

    return result;
}

auto merge_split_segments(const line_tree_vector_t& line_trees) -> std::vector<line2d_t> {
    // merge
    auto segments_merged = merge_segments(line_trees);
    // split
    auto points = to_points_sorted_unique(segments_merged);
    return split_segments(segments_merged, points);
}

template <typename index_t>
auto select_best_root(const AdjacencyGraph<index_t>& graph,
                      std::optional<point2d_t> mandatory,
                      const line_tree_vector_t& line_trees) -> std::optional<point2d_t> {
    // collect candidates
    auto root_candidates = std::vector<point2d_t> {};
    auto is_leaf = [&](index_t index) { return graph.neighbors()[index].size() == 1; };
    // TODO use transform_if
    for (auto index : graph.indices()) {
        if (is_leaf(index)) {
            root_candidates.push_back(graph.points()[index]);
        }
    }

    if (root_candidates.empty()) {
        // no root candiates
        return std::nullopt;
    }

    std::ranges::sort(root_candidates);
    auto has_candiate = [&](point2d_t _root) {
        return std::ranges::binary_search(root_candidates, _root);
    };

    // mandatory
    if (mandatory) {
        if (!has_candiate(*mandatory)) [[unlikely]] {
            // requested root is not possible
            return std::nullopt;
        }
        return *mandatory;
    }

    // TODO use find_if
    for (const auto tree_reference : line_trees) {
        auto input = tree_reference.get().input_point();
        if (has_candiate(input)) {
            return input;
        }
    }

    return root_candidates.at(0);
}

// Merges line tree if possible. With new root, if given.

auto merge(line_tree_vector_t line_trees, std::optional<point2d_t> new_root)
    -> std::optional<LineTree> {
    // trivial cases
    if (std::size(line_trees) == 0) {
        return std::nullopt;
    }
    if (std::size(line_trees) == 1) {
        return line_trees.at(0).get();
    }

    fmt::print("\n");

    const auto merged_segments = merge_split_segments(line_trees);
    const auto graph = LineTree::Graph {merged_segments};

    fmt::print("graph = {}\n", graph);

    if (const auto root = select_best_root(graph, new_root, line_trees)) {
        return LineTree::from_graph(*root, graph);
    }
    return std::nullopt;
}

//
// LineTree
//

LineTree::LineTree(std::initializer_list<point2d_t> points)
    : points_ {points.begin(), points.end()} {
    if (std::size(points) == 1) [[unlikely]] {
        throw_invalid_line_tree_exception("A line tree with one point is invalid.");
    }
    if (std::size(points) == 0) {
        return;
    }

    // indices point to previous point for line
    indices_.resize(std::size(points) - 1);
    std::iota(indices_.begin(), indices_.end(), 0);

    if (!validate_segments_horizontal_or_vertical()) [[unlikely]] {
        throw_invalid_line_tree_exception(
            "Each line segments needs to be horizontal or vertical.");
    }
    if (!validate_horizontal_follows_vertical()) [[unlikely]] {
        throw_invalid_line_tree_exception(
            "Each horizontal segments needs to be followed by a vertical "
            "and vice versa.");
    }
    if (!validate_no_internal_collisions()) [[unlikely]] {
        throw_invalid_line_tree_exception(
            "Lines are not allowed to collide with each other in the graph.");
    }
}

struct LineTree::backtrack_memory_t {
    length_t current_length;
    index_t graph_index;
    index_t last_tree_index;
    uint8_t neighbor_id;
};

auto LineTree::from_graph(point2d_t root, const Graph& graph) -> std::optional<LineTree> {
    // define as optional for RVO
    auto line_tree = std::optional {LineTree {}};

    index_t last_index;
    if (auto res = graph.to_index(root)) {
        last_index = *res;
    } else {
        // root is not part of graph
        return std::nullopt;
    }
    if (graph.neighbors()[last_index].size() != 1) {
        // root element has more than one neighbor
        return std::nullopt;
    }
    auto current_index = graph.neighbors()[last_index][0];
    length_t current_length = 0;

    // depth first search with loop detection
    boost::container::vector<bool> visited(graph.points().size(), false);
    std::vector<backtrack_memory_t> backtrack_vector {};

    // add first element
    line_tree->points_.push_back(graph.points()[last_index]);
    visited[last_index] = true;
    index_t last_tree_index = 0;

    while (true) {
        // check visited
        if (visited[current_index]) {
            // graph contains loops
            return std::nullopt;
        }
        visited[current_index] = true;

        // add current point
        line_tree->points_.push_back(graph.points()[current_index]);
        line_tree->indices_.push_back(last_tree_index);
        current_length
            += distance_1d(graph.points()[current_index], graph.points()[last_index]);

        auto& neighbors = graph.neighbors()[current_index];

        // find next index
        auto next = std::optional<index_t> {};
        if (neighbors.at(0) != last_index) {
            next = neighbors[0];
        } else if (neighbors.size() > 1) {
            assert(neighbors[1] != last_index);
            next = neighbors[1];
        }

        // fmt::print("iteration\n");
        // fmt::print("  tree->points = {}\n", line_tree->points_);
        // fmt::print("  tree->indices = {}\n", line_tree->indices_);
        // fmt::print("  tree->lengths = {}\n", line_tree->lengths_);
        // fmt::print("  last = {} current = {} next = {} neighbors = {}\n", last_index,
        //            current_index, next.value_or(999), neighbors);
        // fmt::print("  backtrack_vector.size() = {}\n", backtrack_vector.size());

        // add backtracking candiates
        for (auto id : range(neighbors.size())) {
            if (neighbors[id] != last_index && (!next || neighbors[id] != *next)) {
                backtrack_vector.push_back(backtrack_memory_t {
                    .current_length = current_length,
                    .graph_index = current_index,
                    .last_tree_index
                    = gsl::narrow_cast<index_t>(line_tree->points_.size() - 1),
                    .neighbor_id = gsl::narrow_cast<uint8_t>(id),
                });
            }
        }

        // choose where to go next
        if (next) {
            // directly connected
            last_index = current_index;
            current_index = *next;
            last_tree_index = gsl::narrow_cast<index_t>(line_tree->points_.size() - 1);
        } else if (!backtrack_vector.empty()) {
            // load next backtracking
            auto& backtrack = backtrack_vector.back();

            last_index = backtrack.graph_index;
            current_index
                = graph.neighbors()[backtrack.graph_index][backtrack.neighbor_id];
            last_tree_index = backtrack.last_tree_index;
            current_length = backtrack.current_length;

            line_tree->lengths_.push_back(current_length);
            backtrack_vector.pop_back();
        } else {
            // we are done
            break;
        }
    }

    if (line_tree->points_.size() < graph.points().size()) {
        // unconnected notes
        return std::nullopt;
    }

    // fmt::print("--done--\n\n\n");
    return line_tree;
}

auto LineTree::reroot(const point2d_t new_root) const -> std::optional<LineTree> {
    if (new_root == input_point()) {
        return *this;
    }

    const auto graph = Graph {segments()};
    return LineTree::from_graph(new_root, graph);
}

auto LineTree::input_point() const -> point2d_t {
    if (points_.size() == 0) [[unlikely]] {
        throw_exception("Empty line tree has no root.");
    }
    return points_[0];
}

auto LineTree::segment_count() const noexcept -> int {
    return gsl::narrow_cast<int>(std::size(indices_));
}

auto LineTree::empty() const noexcept -> bool {
    return points_.empty();
}

auto LineTree::segment(int index) const -> line2d_t {
    return line2d_t {points_.at(indices_.at(index)), points_.at(index + 1)};
}

auto LineTree::segments() const noexcept -> SegmentView {
    return SegmentView(*this);
}

auto LineTree::sized_segments() const noexcept -> SegmentSizeView {
    return SegmentSizeView(*this);
}

auto LineTree::starts_new_subtree(int index) const -> bool {
    if (index == 0) {
        return false;
    }
    return indices_.at(index) != index;
}

auto LineTree::validate_segments_horizontal_or_vertical() const -> bool {
    // TODO why is function itself not possible
    auto test = [](line2d_t line) -> bool { return is_orthogonal(line); };
    return std::all_of(segments().begin(), segments().end(), test);
}

// each horizontal segment is followed by a vertical segment and vice versa
auto LineTree::validate_horizontal_follows_vertical() const -> bool {
    auto is_horizontal = [](line2d_t line) { return line.p0.x == line.p1.x; };
    return std::ranges::adjacent_find(segments().begin(), segments().end(), {},
                                      is_horizontal)
           == segments().end();
}

auto connected_lines_colliding(line2d_t line0, line2d_t line1) -> bool {
    if (line0.p1 == line1.p0) {
        return is_colliding(line0.p0, line1) || is_colliding(line1.p1, line0);
    }
    if (line0.p0 == line1.p0) {
        return is_colliding(line0.p1, line1) || is_colliding(line1.p1, line0);
    }
    [[unlikely]] throw_exception("connected lines need to be ordered differently.");
}

auto LineTree::validate_no_internal_collisions() const -> bool {
    auto are_colliding = [](SegmentIterator it1, SegmentIterator it2) {
        if (it1.is_connected(it2)) {
            return connected_lines_colliding(*it1, *it2);
        }
        return line_points_colliding(*it1, *it2);
    };

    return !has_duplicates_quadratic_custom(segments().begin(), segments().end(),
                                            are_colliding);
}

//
// Exceptions
//

[[noreturn]] auto throw_invalid_line_tree_exception(const char* msg) -> void {
    throw InvalidLineTreeException(msg);
}

InvalidLineTreeException::InvalidLineTreeException(const char* message) noexcept
    : message_ {message} {}

auto InvalidLineTreeException::what() const noexcept -> const char* {
    return message_;
}

//
// SegmentIterator
//

LineTree::SegmentIterator::SegmentIterator(const LineTree& line_tree,
                                           index_t index) noexcept
    : line_tree_ {&line_tree}, index_ {index} {}

auto LineTree::SegmentIterator::operator*() const -> value_type {
    if (line_tree_ == nullptr) [[unlikely]] {
        throw_exception("line tree cannot be null when dereferencing segment iterator");
    }
    return line_tree_->segment(index_);
}

auto LineTree::SegmentIterator::operator++() noexcept -> SegmentIterator& {
    ++index_;
    return *this;
}

auto LineTree::SegmentIterator::operator++(int) noexcept -> SegmentIterator {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

auto LineTree::SegmentIterator::operator--() noexcept -> SegmentIterator& {
    --index_;
    return *this;
}

auto LineTree::SegmentIterator::operator--(int) noexcept -> SegmentIterator {
    auto tmp = *this;
    --(*this);
    return tmp;
}

auto LineTree::SegmentIterator::operator==(const SegmentIterator& right) const noexcept
    -> bool {
    return index_ >= right.index_;
}

auto LineTree::SegmentIterator::operator-(const SegmentIterator& right) const noexcept
    -> difference_type {
    return static_cast<difference_type>(index_) - right.index_;
}

auto LineTree::SegmentIterator::is_connected(const SegmentIterator& other) const noexcept
    -> bool {
    if (index_ == other.index_) {
        return false;
    }
    auto indirectly_connected = [&]() {
        return line_tree_->indices_.at(index_) == line_tree_->indices_.at(other.index_);
    };

    if (index_ < other.index_) {
        return line_tree_->indices_.at(other.index_) == index_ + 1
               || indirectly_connected();
    }
    return line_tree_->indices_.at(index_) == other.index_ + 1 || indirectly_connected();
}

//
// SegmentView
//

LineTree::SegmentView::SegmentView(const LineTree& line_tree) noexcept
    : line_tree_ {&line_tree} {}

auto LineTree::SegmentView::begin() const noexcept -> iterator_type {
    return iterator_type {*line_tree_, 0};
}

auto LineTree::SegmentView::end() const noexcept -> iterator_type {
    return iterator_type {*line_tree_,
                          gsl::narrow_cast<index_t>(line_tree_->segment_count())};
}

auto LineTree::SegmentView::size() const noexcept -> size_t {
    return line_tree_->segment_count();
}

//
// SegmentSizeIterator
//

LineTree::SegmentSizeIterator::SegmentSizeIterator(const LineTree& line_tree,
                                                   index_t point_index) noexcept
    : line_tree_ {&line_tree}, point_index_ {point_index} {}

auto LineTree::SegmentSizeIterator::operator*() const -> value_type {
    if (line_tree_ == nullptr) [[unlikely]] {
        throw_exception("line tree cannot be null when dereferencing segment iterator");
    }

    auto line = line_tree_->segment(point_index_);
    return sized_line2d_t {line, start_length_, start_length_ + distance_1d(line)};
}

auto LineTree::SegmentSizeIterator::operator++() noexcept -> SegmentSizeIterator& {
    if (point_index_ + 1 < line_tree_->indices_.size()
        && line_tree_->starts_new_subtree(point_index_ + 1)) {
        start_length_ = line_tree_->lengths_.at(length_index_++);
    } else {
        start_length_ = (**this).p1_length;
    }
    ++point_index_;

    return *this;
}

auto LineTree::SegmentSizeIterator::operator++(int) noexcept -> SegmentSizeIterator {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

auto LineTree::SegmentSizeIterator::operator==(
    const SegmentSizeIterator& right) const noexcept -> bool {
    return point_index_ >= right.point_index_;
}

auto LineTree::SegmentSizeIterator::operator-(
    const SegmentSizeIterator& right) const noexcept -> difference_type {
    return static_cast<difference_type>(point_index_) - right.point_index_;
}

//
// SegmentSizeView
//

LineTree::SegmentSizeView::SegmentSizeView(const LineTree& line_tree) noexcept
    : line_tree_ {&line_tree} {}

auto LineTree::SegmentSizeView::begin() const noexcept -> iterator_type {
    return iterator_type {*line_tree_, 0};
}

auto LineTree::SegmentSizeView::end() const noexcept -> iterator_type {
    return iterator_type {*line_tree_,
                          gsl::narrow_cast<index_t>(line_tree_->segment_count())};
}

auto LineTree::SegmentSizeView::size() const noexcept -> size_t {
    return line_tree_->segment_count();
}

static_assert(std::bidirectional_iterator<LineTree::SegmentIterator>);
static_assert(std::input_iterator<LineTree::SegmentSizeIterator>);

}  // namespace logicsim
