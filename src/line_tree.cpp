

#include "line_tree.h"

#include "algorithm.h"
#include "collision.h"
#include "exception.h"
#include "format.h"
#include "graph.h"
#include "range.h"
#include "segment_tree.h"

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
    using buffer_t = std::vector<ordered_line_t>;

    SegmentSplitter() {
        buffer_.reserve(16);
    }

    auto split_segment(ordered_line_t segment, std::ranges::input_range auto&& points)
        -> const buffer_t& {
        buffer_.clear();
        buffer_.push_back(segment);

        for (auto point : points) {
            auto splittable = std::ranges::find_if(
                buffer_,
                [&point](ordered_line_t line) -> bool { return is_inside(point, line); });

            if (splittable != buffer_.end()) {
                const auto p0 = splittable->p0;
                const auto p1 = splittable->p1;

                *splittable = ordered_line_t {p0, point};
                buffer_.push_back(ordered_line_t {point, p1});
            }
        }

        return buffer_;
    }

   private:
    buffer_t buffer_ {};
};

auto split_lines(std::ranges::input_range auto&& segments,
                 std::ranges::input_range auto&& points) -> std::vector<ordered_line_t> {
    std::vector<ordered_line_t> result;
    result.reserve(std::size(segments) + std::size(points));

    auto splitter = SegmentSplitter {};
    for (auto segment : segments) {
        std::ranges::copy(splitter.split_segment(segment, points),
                          std::back_inserter(result));
    }
    return result;
}

template <class OutputIterator, class GetterSame, class GetterDifferent>
auto merge_lines_1d(std::span<const ordered_line_t> segments, OutputIterator result,
                    GetterSame get_same, GetterDifferent get_different) -> void {
    // collect lines
    auto parallel_segments = std::vector<ordered_line_t> {};
    parallel_segments.reserve(segments.size());
    transform_if(
        segments, std::back_inserter(parallel_segments),
        [&](ordered_line_t line) -> ordered_line_t { return line; },
        [&](ordered_line_t line) -> bool {
            return get_same(line.p0) == get_same(line.p1);
        });

    // sort lists
    std::ranges::sort(parallel_segments, [&](ordered_line_t a, ordered_line_t b) {
        return std::tie(get_same(a.p0), get_different(a.p0)) <
               std::tie(get_same(b.p0), get_different(b.p0));
    });

    // extract elements
    transform_combine_while(
        parallel_segments, result,
        // make state
        [](auto it) -> ordered_line_t { return *it; },
        // combine while
        [&](ordered_line_t state, auto it) -> bool {
            return get_same(state.p0) == get_same(it->p0) &&
                   get_different(state.p1) >= get_different(it->p0);
        },
        // update state
        [&](ordered_line_t state, auto it) -> ordered_line_t {
            get_different(state.p1) =
                std::max(get_different(state.p1), get_different(it->p1));
            return state;
        });
}

auto merge_lines(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};
    result.reserve(segments.size());

    auto get_x = [](point_t& point) -> grid_t& { return point.x; };
    auto get_y = [](point_t& point) -> grid_t& { return point.y; };

    // vertical & horizontal
    merge_lines_1d(segments, std::back_inserter(result), get_x, get_y);
    merge_lines_1d(segments, std::back_inserter(result), get_y, get_x);

    return result;
}

auto merge_split_segments(std::span<const ordered_line_t> segments)
    -> std::vector<ordered_line_t> {
    // merge
    const auto segments_merged = merge_lines(segments);
    // split
    // TODO can this be simplified?
    const auto points1 = to_points_sorted_unique(segments);
    const auto segments_split = split_lines(segments_merged, points1);
    const auto points2 = points_with_both_orientations(segments_split);
    return split_lines(segments_merged, points2);
}

template <typename index_t>
auto select_best_root(const AdjacencyGraph<index_t>& graph,
                      std::optional<point_t> mandatory,
                      const line_tree_vector_t& line_trees) -> std::optional<point_t> {
    // collect candidates
    auto root_candidates = std::vector<point_t> {};

    auto to_point = [&](index_t index) { return graph.point(index); };
    auto is_leaf = [&](index_t index) { return graph.neighbors()[index].size() == 1; };
    transform_if(graph.indices(), std::back_inserter(root_candidates), to_point, is_leaf);

    if (root_candidates.empty()) {
        // no root candiates
        return std::nullopt;
    }

    std::ranges::sort(root_candidates);
    const auto has_candidate = [&](point_t _root) {
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
    auto to_root = [](auto tree_reference) {
        return tree_reference.get().input_position();
    };
    if (auto result = std::ranges::find_if(line_trees, has_candidate, to_root);
        result != line_trees.end()) {
        return to_root(*result);
    }

    return root_candidates.at(0);
}

auto to_segments(line_tree_vector_t line_trees) -> std::vector<ordered_line_t> {
    auto segments = std::vector<ordered_line_t> {};

    const auto total_count =
        accumulate(transform_view(line_trees, &LineTree::segment_count), std::size_t {0});
    segments.reserve(total_count);

    for (auto&& tree_reference : line_trees) {
        std::ranges::transform(tree_reference.get().segments(),
                               std::back_inserter(segments),
                               [](line_t line) { return ordered_line_t {line}; });
    }

    return segments;
}

auto from_segments_impl(std::span<const ordered_line_t> segments,
                        std::optional<point_t> new_root, line_tree_vector_t line_trees)
    -> std::optional<LineTree> {
    const auto merged_segments = merge_split_segments(segments);
    const auto graph = LineTree::Graph {merged_segments};

    if (const auto root = select_best_root(graph, new_root, line_trees)) {
        return LineTree::from_graph(*root, graph);
    }
    return std::nullopt;
}

// Merges line tree if possible. With new root, if given.

auto merge(line_tree_vector_t line_trees, std::optional<point_t> new_root)
    -> std::optional<LineTree> {
    // trivial cases
    if (std::size(line_trees) == 0) {
        return std::nullopt;
    }
    if (std::size(line_trees) == 1) {
        return line_trees.at(0).get();
    }

    const auto segments = to_segments(line_trees);
    return from_segments_impl(segments, new_root, line_trees);
}

//
// LineTree
//

LineTree::LineTree(std::initializer_list<point_t> points)
    : LineTree {points.begin(), points.end()} {}

auto LineTree::from_segments(std::span<const ordered_line_t> segments,
                             std::optional<point_t> new_root) -> std::optional<LineTree> {
    if (segments.size() == 0) {
        return LineTree {};
    }

    return from_segments_impl(segments, new_root, {});
}

auto LineTree::from_points(std::initializer_list<point_t> points)
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

        // calculate target index
        const auto a_index = line_tree_index_.at(a);
        const auto b_index = gsl::narrow_cast<index_t>(tree_->points_.size());

        if (a_index + 1 != b_index) {  // new subtree?
            tree_->lengths_.push_back(length_recorder_.length(a));

            auto last_index =
                gsl::narrow_cast<index_t>(tree_->points_.size() - std::size_t {1});
            tree_->output_indices_.push_back(last_index);
        }

        line_tree_index_.at(b) = b_index;
        tree_->points_.push_back(graph.point(b));
        tree_->indices_.push_back(a_index);
    };

   private:
    gsl::not_null<LineTree*> tree_;

    LengthRecorderVisitor<index_t, length_t> length_recorder_;
    std::vector<index_t> line_tree_index_ {};
};

auto LineTree::from_graph(point_t root, const Graph& graph) -> std::optional<LineTree> {
    // define as optional for RVO (return value optimization)
    auto line_tree = std::optional {LineTree {}};

    auto root_index = graph.to_index(root);
    if (!root_index) {
        // root is not part of graph
        return std::nullopt;
    }

    auto builder = TreeBuilderVisitor {*line_tree, graph.vertex_count()};
    if (depth_first_search(graph, builder, *root_index) == DFSStatus::success) {
        auto last_index =
            gsl::narrow_cast<index_t>(line_tree->points_.size() - std::size_t {1});
        line_tree->output_indices_.push_back(last_index);
        return line_tree;
    }

    return std::nullopt;
}

auto LineTree::from_segment_tree(const SegmentTree& segment_tree)
    -> std::optional<LineTree> {
    // convert to line_tree
    const auto segments =
        transform_to_vector(segment_tree.segment_infos(),
                            [](const segment_info_t& segment) { return segment.line; });

    const auto root = segment_tree.has_input()
                          ? std::make_optional(segment_tree.input_position())
                          : std::nullopt;
    return LineTree::from_segments(segments, root);
}

auto LineTree::swap(LineTree& other) noexcept -> void {
    points_.swap(other.points_);
    indices_.swap(other.indices_);
    lengths_.swap(other.lengths_);
    output_indices_.swap(other.output_indices_);
}

auto swap(LineTree& a, LineTree& b) noexcept -> void {
    a.swap(b);
}

auto LineTree::sized_line_t::format() const -> std::string {
    return fmt::format("SizedLine({}, {}, {}, {}, {})", line.p0, line.p1, p0_length,
                       p1_length, has_cross_point_p0);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::LineTree& a, logicsim::LineTree& b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

auto LineTree::validate() const -> void {
    if ((points_.size() == 0 && indices_.size() != 0) ||
        (points_.size() > 0 && indices_.size() + 1 != points_.size())) [[unlikely]] {
        throw_exception("indices array has wrong size");
    }
}

auto LineTree::reroot(const point_t new_root) const -> std::optional<LineTree> {
    if (new_root == input_position()) {
        return *this;
    }

    const auto graph = Graph {segments()};
    return LineTree::from_graph(new_root, graph);
}

auto LineTree::input_position() const -> point_t {
    if (points_.size() == 0) [[unlikely]] {
        throw_exception("Empty line tree has no input.");
    }
    return points_[0];
}

auto LineTree::input_orientation() const -> orientation_t {
    if (points_.size() < 2) [[unlikely]] {
        throw_exception("Empty line tree has no input orientation.");
    }
    return to_orientation(points_[1], points_[0]);
}

auto LineTree::segment_count() const noexcept -> std::size_t {
    return std::size(indices_);
}

auto LineTree::empty() const noexcept -> bool {
    return points_.empty();
}

auto LineTree::segment(int index) const -> line_t {
    auto [a, b] = segment_points(index);
    return line_t {a, b};
}

auto LineTree::segment_points(int index) const -> std::pair<point_t, point_t> {
    return {points_.at(indices_.at(index)), points_.at(index + 1)};
}

auto LineTree::segments() const noexcept -> SegmentView {
    return SegmentView {*this};
}

auto LineTree::sized_segments() const noexcept -> SegmentSizeView {
    return SegmentSizeView {*this};
}

auto LineTree::output_count() const -> std::size_t {
    return output_indices_.size();
}

auto LineTree::output_position(std::size_t index) const -> point_t {
    return points_.at(output_indices_.at(index));
}

auto LineTree::output_orientation(std::size_t index) const -> orientation_t {
    const auto segment_index = output_indices_.at(index) - 1;
    const auto line = segment(segment_index);
    return to_orientation(line.p0, line.p1);
}

auto LineTree::calculate_output_lengths() const -> std::vector<length_t> {
    if (segment_count() == 0) {
        return {};
    }
    auto result = std::vector<length_t> {};

    // TODO use algorithm?
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

auto LineTree::points() const -> std::span<const point_t> {
    return points_;
}

auto LineTree::internal_points() const -> InternalPointView {
    return InternalPointView {*this};
}

auto LineTree::has_crosspoint_p0(int index) const -> bool {
    return starts_new_subtree(index);
}

auto LineTree::format() const -> std::string {
    return fmt::format("LineTree({}, {}, {}, {})", points_, indices_, lengths_,
                       output_indices_);
}

// internal

auto LineTree::starts_new_subtree(int index) const -> bool {
    if (index == 0) {
        return false;
    }
    return indices_.at(index) != index;
}

auto LineTree::initialize_data_structure() -> void {
    if (points_.size() <= 1) {
        return;
    }

    // point to previous points for each segment
    indices_.resize(points_.size() - 1);
    std::iota(indices_.begin(), indices_.end(), index_t {0});

    auto last_index = gsl::narrow_cast<index_t>(points_.size() - std::size_t {1});
    output_indices_.push_back(last_index);
}

auto LineTree::validate_points_or_throw() const -> void {
    if (auto error = validate_points_error()) {
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
    const auto is_segment_orthogonal = [this](std::size_t index) {
        auto [p0, p1] = segment_points(gsl::narrow_cast<int>(index));  // TODO remove cast
        return is_orthogonal(p0, p1);
    };
    return std::ranges::all_of(range(segment_count()), is_segment_orthogonal);
}

// each horizontal segment is followed by a vertical segment and vice versa
auto LineTree::validate_horizontal_follows_vertical() const -> bool {
    auto is_horizontal = [](line_t line) { return line.p0.x == line.p1.x; };
    // finds the first two adjacent items that are equal
    return std::ranges::adjacent_find(segments().begin(), segments().end(),
                                      std::ranges::equal_to {},
                                      is_horizontal) == segments().end();
}

auto connected_lines_colliding(line_t line0, line_t line1) -> bool {
    if (line0.p1 == line1.p0) {
        return is_colliding(line0.p0, ordered_line_t {line1}) ||
               is_colliding(line1.p1, ordered_line_t {line0});
    }
    if (line0.p0 == line1.p0) {
        return is_colliding(line0.p1, ordered_line_t {line1}) ||
               is_colliding(line1.p1, ordered_line_t {line0});
    }
    [[unlikely]] throw_exception("connected lines need to be ordered differently.");
}

auto LineTree::validate_no_internal_collisions() const -> bool {
    auto are_colliding = [](SegmentIterator it0, SegmentIterator it1) {
        if (it0.is_connected(it1)) {
            return connected_lines_colliding(*it0, *it1);
        }
        return line_points_colliding(ordered_line_t {*it0}, ordered_line_t {*it1});
    };
    return !has_duplicates_quadratic_iterator(segments().begin(), segments().end(),
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
    operator++();
    return tmp;
}

auto LineTree::SegmentIterator::operator--() noexcept -> SegmentIterator& {
    --index_;
    return *this;
}

auto LineTree::SegmentIterator::operator--(int) noexcept -> SegmentIterator {
    auto tmp = *this;
    operator--();
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
        return line_tree_->indices_.at(other.index_) == index_ + 1 ||
               indirectly_connected();
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
    return iterator_type {*line_tree_, gsl::narrow_cast<index_t>(size())};
}

auto LineTree::SegmentView::size() const noexcept -> size_t {
    return line_tree_->segment_count();
}

//
// Internal Point Iterator
//

LineTree::InternalPointIterator::InternalPointIterator(const LineTree& line_tree,
                                                       index_t index) noexcept
    : line_tree_ {&line_tree}, index_ {index} {}

auto LineTree::InternalPointIterator::operator*() const -> value_type {
    if (line_tree_ == nullptr) [[unlikely]] {
        throw_exception("line tree cannot be null when dereferencing segment iterator");
    }
    return line_tree_->points_.at(index_);
}

auto LineTree::InternalPointIterator::operator++() noexcept -> InternalPointIterator& {
    ++index_;

    while (index_ < line_tree_->segment_count() &&
           line_tree_->starts_new_subtree(index_)) {
        ++index_;
    }

    return *this;
}

auto LineTree::InternalPointIterator::operator++(int) noexcept -> InternalPointIterator {
    auto tmp = *this;
    operator++();
    return tmp;
}

auto LineTree::InternalPointIterator::operator==(
    const InternalPointIterator& right) const noexcept -> bool {
    return index_ >= right.index_;
}

//
// Internal Point View
//

LineTree::InternalPointView::InternalPointView(const LineTree& line_tree) noexcept
    : line_tree_ {&line_tree} {}

auto LineTree::InternalPointView::begin() const noexcept -> iterator_type {
    return iterator_type {*line_tree_, 1};
}

auto LineTree::InternalPointView::end() const noexcept -> iterator_type {
    if (line_tree_->points_.size() == 0) {
        return begin();
    }
    return iterator_type {*line_tree_, gsl::narrow_cast<index_t>(
                                           line_tree_->points_.size() - std::size_t {1})};
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
    return sized_line_t {line, start_length_, start_length_ + distance(line),
                         has_connector};
}

auto LineTree::SegmentSizeIterator::operator++() noexcept -> SegmentSizeIterator& {
    if (point_index_ + index_t {1} < std::ssize(line_tree_->indices_) &&
        line_tree_->starts_new_subtree(point_index_ + 1)) {
        start_length_ = line_tree_->lengths_.at(length_index_++);
    } else {
        start_length_ = (**this).p1_length;
    }

    ++point_index_;
    return *this;
}

auto LineTree::SegmentSizeIterator::operator++(int) noexcept -> SegmentSizeIterator {
    const auto tmp = *this;
    operator++();
    return tmp;
}

auto LineTree::SegmentSizeIterator::operator==(
    const SegmentSizeIterator& right) const noexcept -> bool {
    return point_index_ >= right.point_index_;
}

auto LineTree::SegmentSizeIterator::operator-(
    const SegmentSizeIterator& right) const noexcept -> difference_type {
    return static_cast<difference_type>(point_index_) -
           static_cast<difference_type>(right.point_index_);
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
