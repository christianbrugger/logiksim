#include "segment_tree.h"

#include "line_tree.h"

namespace logicsim {

auto format(SegmentPointType type) -> std::string {
    switch (type) {
        using enum SegmentPointType;

        case normal:
            return "normal";
        case input:
            return "input";
        case output:
            return "output";
        case cross_point:
            return "cross_point";
    }
    throw_exception("Don't know how to convert SegmentPointType to string.");
}

SegmentTree::SegmentTree(SegmentInfo segment) {
    add_segment(segment);
}

auto SegmentTree::swap(SegmentTree& other) noexcept -> void {
    using std::swap;

    segments_.swap(other.segments_);
    cross_points_.swap(other.cross_points_);
    output_positions_.swap(other.output_positions_);

    swap(input_position_, other.input_position_);
    swap(has_input_, other.has_input_);
}

auto swap(SegmentTree& a, SegmentTree& b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::SegmentTree& a, logicsim::SegmentTree& b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

auto SegmentTree::add_segment(SegmentInfo segment) -> void {
    for (auto [type, point] : {
             std::pair {segment.p0_type, segment.line.p0},
             std::pair {segment.p1_type, segment.line.p1},
         }) {
        switch (type) {
            using enum SegmentPointType;
            case normal: {
                break;
            }
            case input: {
                if (has_input_) [[unlikely]] {
                    throw_exception("Segment tree already has one input.");
                }
                has_input_ = true;
                input_position_ = point;
                break;
            }
            case output: {
                output_positions_.push_back(point);
                break;
            }
            case cross_point: {
                cross_points_.push_back(point);
                break;
            }
        }
    }
    segments_.push_back(segment.line);
}

auto SegmentTree::add_tree(const SegmentTree& tree) -> void {
    if (tree.has_input_) {
        if (has_input_) [[unlikely]] {
            throw_exception("Merged tree cannot have two inputs");
        }
        has_input_ = true;
        input_position_ = tree.input_position_;
    }

    output_positions_.insert(output_positions_.end(), tree.output_positions_.begin(),
                             tree.output_positions_.end());
    segments_.insert(segments_.end(), tree.segments_.begin(), tree.segments_.end());
}

auto SegmentTree::empty() const noexcept -> bool {
    return segments_.empty();
}

auto SegmentTree::segment_count() const noexcept -> std::size_t {
    return segments_.size();
}

auto SegmentTree::segment(std::size_t index) const -> line_t {
    return segments_.at(index);
}

auto SegmentTree::segments() const -> std::span<const line_t> {
    return segments_;
}

auto SegmentTree::has_input() const noexcept -> bool {
    return has_input_;
}

auto SegmentTree::input_position() const -> point_t {
    if (!has_input_) {
        throw_exception("Segment tree has no input.");
    }
    return input_position_;
}

auto SegmentTree::cross_points() const -> std::span<const point_t> {
    return cross_points_;
}

auto SegmentTree::output_count() const noexcept -> std::size_t {
    return output_positions_.size();
}

auto SegmentTree::output_positions() const -> std::span<const point_t> {
    return output_positions_;
}

auto SegmentTree::output_position(std::size_t index) const -> point_t {
    return output_positions_.at(index);
}

auto SegmentTree::format() const -> std::string {
    const auto input_format = !has_input_ ? "" : fmt::format(", {}", input_position_);
    return fmt::format("SegmentTree({}, {}{})", segments_, input_format,
                       output_positions_);
}

auto SegmentTree::verify() const -> void {
    const auto new_root = has_input_ ? std::make_optional(input_position_) : std::nullopt;
    const auto line_tree = LineTree::from_segments(segments_, new_root);

    if (!line_tree.has_value()) [[unlikely]] {
        throw_exception("Invalid Segment Tree.");
    }

    // TODO verify outputs
    // TODO verify cross points
}

}  // namespace logicsim
