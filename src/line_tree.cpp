

#include "line_tree.h"

#include "algorithm.h"
#include "collision.h"
#include "exceptions.h"
#include "format.h"
#include "range.h"

#include <boost/container/static_vector.hpp>
#include <gsl/gsl>

#include <algorithm>
#include <numeric>

namespace logicsim {

LineTree::LineTree(std::initializer_list<point2d_t> points)
    : points_ {points.begin(), points.end()} {
    if (std::size(points) == 1) [[unlikely]] {
        throw_invalid_line_tree_exception("A line tree with one point is invalid.");
    }

    // indices point to previous point for line
    indices_.resize(segment_count());
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

// TODO return type with error message
auto LineTree::merge(const LineTree& other, std::optional<point2d_t> new_root) const
    -> LineTree {
    // trivial cases
    if (this->empty()) {
        return other;
    }
    if (other.empty()) {
        return *this;
    }

    // collect all points
    std::vector<point2d_t> points {};
    points.reserve(std::size(points_) + std::size(other.points_));
    points.insert(points.end(), this->points_.begin(), this->points_.end());
    points.insert(points.end(), other.points_.begin(), other.points_.end());

    // remove duplicates & sort
    auto order = [](point2d_t a, point2d_t b) {
        return std::tie(a.x, a.y) < std::tie(b.x, b.y);
    };
    std::ranges::sort(points, order);
    points.erase(std::ranges::unique(points).begin(), points.end());

    // adjacency list (each point can only have 4 neighbors)
    using adjacency_t = boost::container::static_vector<index_t, 4>;
    std::vector<adjacency_t> neighbors(points.size());

    // add all segments
    auto to_index = [&](point2d_t _point) {
        return std::ranges::lower_bound(points, _point, order) - points.begin();
    };
    auto add_segments = [&neighbors, &to_index](const SegmentView _segment_view) {
        for (auto segment : _segment_view) {
            auto index0 = to_index(segment.p0);
            auto index1 = to_index(segment.p1);

            neighbors.at(index0).push_back(gsl::narrow_cast<index_t>(index1));
            neighbors.at(index1).push_back(gsl::narrow_cast<index_t>(index0));
        }
    };
    add_segments(this->segments());
    add_segments(other.segments());

    // find intra line collisions
    // TODO implement

    // remove merged straight lines / unnecessary points
    // TODO implement

    // find possible roots = points with only one neighbor
    std::vector<point2d_t> root_candidates;
    for (auto index : range(neighbors.size())) {
        if (neighbors[index].size() == 1) {
            root_candidates.push_back(points[index]);
        }
    }
    std::ranges::sort(root_candidates, order);
    auto has_candiate = [&](point2d_t _root) {
        return std::ranges::binary_search(root_candidates, _root, order);
    };

    fmt::print("\n");
    fmt::print("points          = {}\n", points);
    fmt::print("neighbors       = {}\n", neighbors);
    fmt::print("root candidates = {}\n", root_candidates);

    // decide new root
    if (root_candidates.empty()) {
        throw_exception("Merged line tree has no root candiates.");
    }
    if (new_root) {
        if (!has_candiate(*new_root)) [[unlikely]] {
            throw_exception("Requested root is not possible for the merged line tree.");
        }
        fmt::print(" -> given root {}\n", *new_root);
    }
    if (has_candiate(root())) {
        fmt::print(" -> this root {}\n", root());
        // return LineTree {};
    }
    if (has_candiate(other.root())) {
        fmt::print(" -> other root {}\n", other.root());
        // return LineTree {};
    }

    return LineTree {};
}

auto LineTree::reroot(const point2d_t new_root) const -> LineTree {
    return LineTree {};
}

auto LineTree::root() const -> point2d_t {
    if (points_.size() == 0) [[unlikely]] {
        throw_exception("Empty line tree has no root.");
    }
    return points_[0];
}

auto LineTree::segment_count() const noexcept -> int {
    return gsl::narrow_cast<int>(std::size(points_) == 0 ? 0 : std::size(points_) - 1);
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
        return point_in_line(line0.p0, line1) || point_in_line(line1.p1, line0);
    }
    if (line0.p0 == line1.p0) {
        return point_in_line(line0.p1, line1) || point_in_line(line1.p1, line0);
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

auto LineTree::validate_no_unnecessary_points() const -> bool {
    // no duplicate edges for merges
    return true;
}

auto LineTree::validate() const -> bool {
    if (std::size(indices_) == 0 && std::size(points_) == 0) {
        return true;
    }
    assert(std::size(indices_) + 1 == std::size(points_));

    // root node needs exactly one child
    if (std::ranges::count_if(indices_, [](auto v) { return v == 0; }) != 1) {
        return false;
    }

    if (!validate_segments_horizontal_or_vertical()) {
        return false;
    }

    if (!validate_no_internal_collisions()) {
        return false;
    }

    // no unconnected nodes

    // contains no cycle
    // cycles require that an index points to a node to its right
    for (auto i : range(std::size(indices_))) {
        if (indices_[i] > i) {
            return false;
        }
    }

    return true;
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

//
// SegmentSizeIterator
//

LineTree::SegmentSizeIterator::SegmentSizeIterator(const LineTree& line_tree,
                                                   index_t index,
                                                   length_t start_length) noexcept
    : line_tree_ {&line_tree}, start_length_ {start_length}, index_ {index} {}

auto LineTree::SegmentSizeIterator::operator*() const -> value_type {
    if (line_tree_ == nullptr) [[unlikely]] {
        throw_exception("line tree cannot be null when dereferencing segment iterator");
    }

    auto line = line_tree_->segment(index_);
    return sized_line2d_t {line, start_length_, start_length_ + distance_1d(line)};
}

auto LineTree::SegmentSizeIterator::operator++() noexcept -> SegmentSizeIterator& {
    start_length_ = (**this).p1_length;
    ++index_;
    return *this;
}

auto LineTree::SegmentSizeIterator::operator++(int) noexcept -> SegmentSizeIterator {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

auto LineTree::SegmentSizeIterator::operator==(
    const SegmentSizeIterator& right) const noexcept -> bool {
    return index_ >= right.index_;
}

auto LineTree::SegmentSizeIterator::operator-(
    const SegmentSizeIterator& right) const noexcept -> difference_type {
    return static_cast<difference_type>(index_) - right.index_;
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

static_assert(std::bidirectional_iterator<LineTree::SegmentIterator>);
static_assert(std::input_iterator<LineTree::SegmentSizeIterator>);

}  // namespace logicsim
