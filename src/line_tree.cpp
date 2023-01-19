

#include "line_tree.h"

#include "exceptions.h"
#include "range.h"

#include <gsl/gsl>

#include <algorithm>
#include <numeric>

namespace logicsim {

LineTree::LineTree(std::initializer_list<point2d_t> points)
    : points_ {points.begin(), points.end()} {
    if (std::size(points_) == 1) [[unlikely]] {
        throw_exception("A line tree with one point is invalid.");
    }

    // indices point to corresponding pair
    indices_.resize(segment_count());
    std::iota(indices_.begin(), indices_.end(), 0);

    if (!validate_segments_horizontal_or_vertical()) [[unlikely]] {
        throw_exception("Each line segments needs to be horizontal or vertical.");
    }
    if (!validate_no_internal_collisions()) [[unlikely]] {
        throw_exception("Lines are not allowed to collide with each other in the graph.");
    }
}

auto LineTree::segment_count() const noexcept -> int {
    return gsl::narrow_cast<int>(std::size(points_) == 0 ? 0 : std::size(points_) - 1);
}

auto LineTree::segment(int index) const -> line2d_t {
    return line2d_t {points_.at(indices_.at(index)), points_.at(index + 1)};
}

auto LineTree::segments() const noexcept -> SegmentView {
    return SegmentView(*this);
}

auto LineTree::validate_segments_horizontal_or_vertical() const -> bool {
    return true;
}

auto LineTree::validate_no_internal_collisions() const -> bool {
    // no duplicate edges
    // edges not colliding with other points

    return true;
}

auto LineTree::validate_no_unecessary_points() const -> bool {
    // no duplicate edges
    // edges not colliding with other points

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
    ++index_;
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

}  // namespace logicsim
