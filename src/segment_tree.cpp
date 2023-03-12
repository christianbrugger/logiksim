#include "segment_tree.h"

#include "line_tree.h"

namespace logicsim {

auto format(SegmentPointType type) -> std::string {
    switch (type) {
        using enum SegmentPointType;

        case input:
            return "input";
        case output:
            return "output";
        case colliding_point:
            return "mid_point";
        case shadow_point:
            return "shadow_point";
        case cross_point:
            return "cross_point";
        case new_unknown:
            return "new_unknown";
    }
    throw_exception("Don't know how to convert SegmentPointType to string.");
}

auto segment_info_t::format() const -> std::string {
    return fmt::format("Segment({} {} - {} {})", p0_type, line.p0, line.p1, p1_type);
}

//
// Segment Tree
//

SegmentTree::SegmentTree(segment_info_t segment) {
    add_segment(segment);
}

auto SegmentTree::swap(SegmentTree& other) noexcept -> void {
    using std::swap;

    segments_.swap(other.segments_);

    swap(output_count_, other.output_count_);
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

auto SegmentTree::register_segment(segment_info_t segment) -> void {
    for (auto [type, point] : {
             std::pair {segment.p0_type, segment.line.p0},
             std::pair {segment.p1_type, segment.line.p1},
         }) {
        switch (type) {
            using enum SegmentPointType;

            case input: {
                if (has_input_) [[unlikely]] {
                    throw_exception("Segment tree already has one input.");
                }
                has_input_ = true;
                input_position_ = point;
                break;
            }

            case output: {
                ++output_count_;
                break;
            }

            case colliding_point:
            case shadow_point:
            case cross_point:
            case new_unknown: {
                break;
            }
        }
    }
}

auto SegmentTree::unregister_segment(segment_info_t segment) -> void {
    for (auto [type, point] : {
             std::pair {segment.p0_type, segment.line.p0},
             std::pair {segment.p1_type, segment.line.p1},
         }) {
        switch (type) {
            using enum SegmentPointType;

            case input: {
                if (!has_input_) [[unlikely]] {
                    throw_exception("Tree should have input thats not present.");
                }
                has_input_ = false;
                input_position_ = {};
                break;
            }

            case output: {
                if (output_count_ <= 0) [[unlikely]] {
                    throw_exception("Tree should have output thats not present.");
                }
                --output_count_;
                break;
            }

            case colliding_point:
            case shadow_point:
            case cross_point:
            case new_unknown: {
                break;
            }
        }
    }
}

auto SegmentTree::add_segment(segment_info_t segment) -> segment_index_t {
    const auto new_index
        = segment_index_t {gsl::narrow<segment_index_t::value_type>(segments_.size())};

    register_segment(segment);
    segments_.push_back(segment);

    return new_index;
}

auto SegmentTree::add_tree(const SegmentTree& tree) -> void {
    if (tree.has_input_) {
        if (has_input_) [[unlikely]] {
            throw_exception("Merged tree cannot have two inputs");
        }
        has_input_ = true;
        input_position_ = tree.input_position_;
    }

    output_count_ += tree.output_count_;
    segments_.insert(segments_.end(), tree.segments_.begin(), tree.segments_.end());
}

auto SegmentTree::update_segment(segment_index_t index, segment_info_t segment) -> void {
    auto& entry = segments_.at(index.value);

    unregister_segment(entry);
    register_segment(segment);

    entry = segment;
}

auto SegmentTree::empty() const noexcept -> bool {
    return segments_.empty();
}

auto SegmentTree::segment_count() const noexcept -> std::size_t {
    return segments_.size();
}

auto SegmentTree::segment(std::size_t index) const -> segment_info_t {
    return segments_.at(index);
}

auto SegmentTree::segment(segment_index_t index) const -> segment_info_t {
    return segments_.at(index.value);
}

auto SegmentTree::segments() const -> std::span<const segment_info_t> {
    return segments_;
}

auto SegmentTree::has_input() const noexcept -> bool {
    return has_input_;
}

auto SegmentTree::input_count() const noexcept -> std::size_t {
    return has_input_ ? 1 : 0;
}

auto SegmentTree::input_position() const -> point_t {
    if (!has_input_) {
        throw_exception("Segment tree has no input.");
    }
    return input_position_;
}

auto SegmentTree::output_count() const noexcept -> std::size_t {
    return output_count_;
}

auto SegmentTree::format() const -> std::string {
    return fmt::format("SegmentTree({}x{}, {})", input_count(), output_count(),
                       segments_);
}

auto SegmentTree::verify() const -> void {
    const auto new_root = has_input_ ? std::make_optional(input_position_) : std::nullopt;

    // TODO optimize this?
    const auto segments = transform_to_vector(
        segments_, [](const segment_info_t& segment) { return segment.line; });
    const auto line_tree = LineTree::from_segments(segments, new_root);

    if (!line_tree.has_value()) [[unlikely]] {
        throw_exception("Invalid Segment Tree.");
    }

    // TODO verify outputs
    // TODO verify cross points
}

}  // namespace logicsim
