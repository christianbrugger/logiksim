#include "segment_tree.h"

#include "line_tree.h"
#include "range.h"

namespace logicsim {

auto format(SegmentPointType type) -> std::string {
    switch (type) {
        using enum SegmentPointType;

        case input:
            return "input";
        case output:
            return "output";
        case colliding_point:
            return "colliding_point";
        case cross_point_horizontal:
            return "cross_point_horizontal";
        case cross_point_vertical:
            return "cross_point_vertical";

        case shadow_point:
            return "shadow_point";
        case visual_cross_point:
            return "visual_cross_point";
        case new_unknown:
            return "new_unknown";
    }
    throw_exception("Don't know how to convert SegmentPointType to string.");
}

auto is_cross_point(SegmentPointType point_type) -> bool {
    using enum SegmentPointType;
    return point_type == cross_point_horizontal || point_type == cross_point_vertical
           || point_type == visual_cross_point;
}

auto order_points(segment_info_t a, segment_info_t b)
    -> std::tuple<segment_info_t, segment_info_t> {
    if (a.line <= b.line) {
        return std::make_tuple(a, b);
    }
    return std::make_tuple(b, a);
}

auto segment_info_t::format() const -> std::string {
    return fmt::format("Segment({} {} - {} {})", p0_type, line.p0, line.p1, p1_type);
}

//
// Segment Tree
//

auto SegmentTree::swap(SegmentTree& other) noexcept -> void {
    using std::swap;

    segments_.swap(other.segments_);
    valid_parts_vector_.swap(other.valid_parts_vector_);

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
auto SegmentTree::get_next_index() const -> segment_index_t {
    return segment_index_t {gsl::narrow<segment_index_t::value_type>(segments_.size())};
}

auto SegmentTree::register_segment(segment_index_t index) -> void {
    const auto& segment = segments_.at(index.value);

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
            case cross_point_horizontal:
            case cross_point_vertical:
            case shadow_point:
            case visual_cross_point:
            case new_unknown: {
                break;
            }
        }
    }
}

auto SegmentTree::unregister_segment(segment_index_t index) -> void {
    const auto& segment = segments_.at(index.value);

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
                if (point != input_position_) [[unlikely]] {
                    throw_exception(
                        "Removed segment has wrong input position as stored.");
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
            case cross_point_horizontal:
            case cross_point_vertical:
            case shadow_point:
            case visual_cross_point:
            case new_unknown: {
                break;
            }
        }
    }
}

auto SegmentTree::delete_last_segment() -> void {}

auto SegmentTree::add_segment(segment_info_t segment) -> segment_index_t {
    const auto new_index = get_next_index();

    segments_.push_back(segment);
    valid_parts_vector_.push_back(parts_vector_t {});
    register_segment(new_index);

    return new_index;
}

auto SegmentTree::swap_and_delete_segment(segment_index_t index) -> void {
    if (segments_.empty()) [[unlikely]] {
        throw_exception("Cannot delete from empty segment tree.");
    }

    const auto last_index = this->last_index();
    segments_.at(index.value) = segments_.at(last_index.value);
    valid_parts_vector_.at(index.value).swap(valid_parts_vector_.at(last_index.value));

    // delete
    unregister_segment(last_index);
    segments_.pop_back();
    valid_parts_vector_.pop_back();
}

auto SegmentTree::add_tree(const SegmentTree& tree) -> segment_index_t {
    const auto next_index = get_next_index();

    if (tree.has_input_) {
        if (has_input_) [[unlikely]] {
            throw_exception("Merged tree cannot have two inputs");
        }
        has_input_ = true;
        input_position_ = tree.input_position_;
    }

    output_count_ += tree.output_count_;
    segments_.insert(segments_.end(), tree.segments_.begin(), tree.segments_.end());
    valid_parts_vector_.insert(valid_parts_vector_.end(),
                               tree.valid_parts_vector_.begin(),
                               tree.valid_parts_vector_.end());

    return next_index;
}

auto SegmentTree::update_segment(segment_index_t index, segment_info_t segment) -> void {
    unregister_segment(index);
    segments_.at(index.value) = segment;
    register_segment(index);
}

auto SegmentTree::empty() const noexcept -> bool {
    return segments_.empty();
}

auto SegmentTree::segment_count() const noexcept -> std::size_t {
    return segments_.size();
}

auto SegmentTree::segment_info(std::size_t index) const -> segment_info_t {
    return segments_.at(index);
}

auto SegmentTree::segment_info(segment_index_t index) const -> segment_info_t {
    return segments_.at(index.value);
}

auto SegmentTree::segment_line(std::size_t index) const -> ordered_line_t {
    return segment_info(index).line;
}

auto SegmentTree::segment_line(segment_index_t index) const -> ordered_line_t {
    return segment_info(index).line;
}

auto SegmentTree::segment_infos() const -> std::span<const segment_info_t> {
    return segments_;
}

auto SegmentTree::mark_valid(segment_index_t segment_index, part_t part) -> void {
    auto& valid_parts = valid_parts_vector_.at(segment_index.value);
    add_part(valid_parts, part);
}

auto SegmentTree::unmark_valid(segment_index_t segment_index, part_t part) -> void {
    auto& valid_parts = valid_parts_vector_.at(segment_index.value);
    remove_part(valid_parts, part);
}

auto SegmentTree::valid_parts() const -> std::span<const parts_vector_t> {
    return valid_parts_vector_;
}

auto SegmentTree::valid_parts(segment_index_t segment_index) const
    -> std::span<const part_t> {
    return valid_parts_vector_.at(segment_index.value);
}

auto SegmentTree::first_index() const noexcept -> segment_index_t {
    return segment_index_t {0};
}

auto SegmentTree::last_index() const noexcept -> segment_index_t {
    const auto result = segment_count() - std::size_t {1};
    return segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(result)};
}

auto SegmentTree::indices() const noexcept -> forward_range_t<segment_index_t> {
    const auto count = segment_index_t {
        gsl::narrow_cast<segment_index_t::value_type>(segment_count())};
    return range(count);
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
    return fmt::format("SegmentTree({}x{}, {}, valid {})", input_count(), output_count(),
                       segments_, valid_parts_vector_);
}

auto SegmentTree::validate_inserted() const -> void {
    validate();

    // convert to line_tree
    const auto segments = transform_to_vector(
        segments_, [](const segment_info_t& segment) { return segment.line; });

    const auto new_root = has_input_ ? std::make_optional(input_position_) : std::nullopt;
    const auto line_tree = LineTree::from_segments(segments, new_root);
    if (!line_tree.has_value()) [[unlikely]] {
        throw_exception("Invalid Segment Tree.");
    }

    // TODO verify
    // - segment p0_type & p1_type
    //
    // - has_input_
    // - output_count_
    //
    // - input & output points
    // - cross points
}

auto SegmentTree::validate() const -> void {
    if (valid_parts_vector_.size() != segments_.size()) [[unlikely]] {
        throw_exception("Vector sizes don't match in segment tree.");
    }
    if ((has_input_ ? 1 : 0) + std::size_t {output_count_}
        > std::size_t {segments_.size()} + 1) [[unlikely]] {
        throw_exception("To many inputs / outputs.");
    }

    // valid parts
    for (auto index : indices()) {
        validate_segment_parts(valid_parts(index), segment_line(index));
    }
}

}  // namespace logicsim
