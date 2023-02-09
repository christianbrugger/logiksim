

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
#include <ranges>
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

    auto to_point = [&](index_t index) { return graph.point(index); };
    auto is_leaf = [&](index_t index) { return graph.neighbors()[index].size() == 1; };
    transform_if(graph.indices(), std::back_inserter(root_candidates), to_point, is_leaf);

    if (root_candidates.empty()) {
        // no root candiates
        return std::nullopt;
    }

    std::ranges::sort(root_candidates);
    const auto has_candidate = [&](point2d_t _root) {
        return std::ranges::binary_search(root_candidates, _root);
    };

    // mandatory
    if (mandatory) {
        if (!has_candidate(*mandatory)) [[unlikely]] {
            // requested root is not possible
            return std::nullopt;
        }
        return *mandatory;
    }

    // original line_tree roots
    auto to_root = [](auto tree_reference) { return tree_reference.get().input_point(); };
    if (auto result = std::ranges::find_if(line_trees, has_candidate, to_root);
        result != line_trees.end()) {
        return to_root(*result);
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

    const auto merged_segments = merge_split_segments(line_trees);
    const auto graph = LineTree::Graph {merged_segments};

    if (const auto root = select_best_root(graph, new_root, line_trees)) {
        return LineTree::from_graph(*root, graph);
    }
    return std::nullopt;
}

//
// LineTree
//

LineTree::LineTree(std::initializer_list<point2d_t> points)
    : LineTree {points.begin(), points.end()} {}

auto LineTree::from_points(std::initializer_list<point2d_t> points)
    -> std::optional<LineTree> {
    return LineTree::from_points(points.begin(), points.end());
}

class LineTree::TreeBuilderVisitor {
   public:
    TreeBuilderVisitor(LineTree& tree, index_t vertex_count)
        : tree_ {&tree}, length_recorder_ {vertex_count}, line_tree_index_(vertex_count) {
        if (vertex_count > 0) {
            tree_->points_.reserve(vertex_count);
            tree_->indices_.reserve(vertex_count - 1);
        }
    }

    auto tree_edge(index_t a, index_t b, AdjacencyGraph<index_t> graph) -> void {
        length_recorder_.tree_edge(a, b, graph);

        if (tree_->points_.empty()) {
            tree_->points_.push_back(graph.point(a));
        }

        auto a_index = line_tree_index_.at(a);
        auto b_index = gsl::narrow_cast<index_t>(tree_->points_.size());

        line_tree_index_.at(b) = b_index;
        tree_->points_.push_back(graph.point(b));
        tree_->indices_.push_back(a_index);

        if (a_index + 1 != b_index) {  // new subtree?
            tree_->lengths_.push_back(length_recorder_.length(a));
        }
    };

   private:
    gsl::not_null<LineTree*> tree_;

    LengthRecorderVisitor<index_t, length_t> length_recorder_;
    std::vector<index_t> line_tree_index_ {};
};

auto LineTree::from_graph(point2d_t root, const Graph& graph) -> std::optional<LineTree> {
    // define as optional for RVO (return value optimization)
    auto line_tree = std::optional {LineTree {}};

    auto root_index = graph.to_index(root);
    if (!root_index) {
        // root is not part of graph
        return std::nullopt;
    }

    auto builder = TreeBuilderVisitor {*line_tree, graph.vertex_count()};
    if (depth_first_search(graph, builder, *root_index) == DFSResult::success) {
        return line_tree;
    }

    return std::nullopt;
}

auto LineTree::swap(LineTree& other) noexcept -> void {
    points_.swap(other.points_);
    indices_.swap(other.indices_);
    lengths_.swap(other.lengths_);
}

auto swap(LineTree& a, LineTree& b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::LineTree& a, logicsim::LineTree& b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

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

auto LineTree::output_count() const -> std::size_t {
    if (segment_count() == 0) {
        return 0;
    }

    auto count = adjacent_count_if(segments(), [](line2d_t first, line2d_t second) {
        return first.p1 != second.p0;
    });
    return count + 1;
}

auto LineTree::output_delays() const -> std::vector<length_t> {
    if (segment_count() == 0) {
        return {};
    }
    auto result = std::vector<length_t> {};

    auto next = sized_segments().begin();
    auto last = sized_segments().end();

    assert(next != last);
    auto last_val = *next;
    ++next;

    for (; next != last; ++next) {
        auto next_val = *next;

        if (last_val.line.p1 != next_val.line.p0) {
            result.push_back(last_val.p1_length);
        }
        last_val = next_val;
    }
    result.push_back(last_val.p1_length);
    return result;
}

auto LineTree::format() const -> std::string {
    return fmt::format("LineTree({}, {}, {})", points_, indices_, lengths_);
}

// internal

auto LineTree::starts_new_subtree(int index) const -> bool {
    if (index == 0) {
        return false;
    }
    return indices_.at(index) != index;
}

auto LineTree::initialize_indices() -> void {
    if (points_.size() <= 1) {
        return;
    }

    // point to previous points for each segment
    indices_.resize(points_.size() - 1);
    std::iota(indices_.begin(), indices_.end(), 0);
}

auto LineTree::validate_points_or_throw() const -> void {
    if (auto error = validate_points_error(); error.has_value()) {
        throw std::move(error.value());
    }
}

auto LineTree::validate_points_error() const -> std::optional<InvalidLineTreeException> {
    using make_result = std::optional<InvalidLineTreeException>;

    if (points_.size() == 1) {
        return make_result {"A line tree with one point is invalid."};
    }

    if (!validate_segments_horizontal_or_vertical()) {
        return make_result {"Each line segments needs to be horizontal or vertical."};
    }
    if (!validate_horizontal_follows_vertical()) {
        return make_result {
            "Each horizontal segments needs to be followed by a vertical "
            "and vice versa."};
    }
    if (!validate_no_internal_collisions()) {
        return make_result {
            "Lines are not allowed to collide with each other in the graph."};
    }
    return std::nullopt;
}

auto LineTree::validate_segments_horizontal_or_vertical() const -> bool {
    return std::ranges::all_of(segments(), is_orthogonal);
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

    // TODO why do we need custom here?
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
    bool has_connector = line_tree_->starts_new_subtree(point_index_);
    return sized_line2d_t {line, start_length_, start_length_ + distance_1d(line),
                           has_connector};
}

auto LineTree::SegmentSizeIterator::operator++() noexcept -> SegmentSizeIterator& {
    if (point_index_ + index_t {1} < line_tree_->indices_.size()
        && line_tree_->starts_new_subtree(point_index_ + 1)) {
        start_length_ = line_tree_->lengths_.at(length_index_++);
    } else {
        start_length_ = (**this).p1_length;
    }
    ++point_index_;

    return *this;
}

auto LineTree::SegmentSizeIterator::operator++(int) noexcept -> SegmentSizeIterator {
    const auto tmp = *this;
    ++(*this);
    return tmp;
}

auto LineTree::SegmentSizeIterator::operator==(
    const SegmentSizeIterator& right) const noexcept -> bool {
    return point_index_ >= right.point_index_;
}

auto LineTree::SegmentSizeIterator::operator-(
    const SegmentSizeIterator& right) const noexcept -> difference_type {
    return static_cast<difference_type>(point_index_)
           - static_cast<difference_type>(right.point_index_);
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
