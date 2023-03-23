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
            return "colliding_point";

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

auto SegmentTree::swap(SegmentTree& other) noexcept -> void {
    using std::swap;

    segments_.swap(other.segments_);
    display_states_.swap(other.display_states_);

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
    if (!is_collision_considered(display_states_.at(index.value))) {
        return;
    }
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
            case shadow_point:
            case cross_point:
            case new_unknown: {
                break;
            }
        }
    }
}

auto SegmentTree::unregister_segment(segment_index_t index) -> void {
    if (!is_collision_considered(display_states_.at(index.value))) {
        return;
    }
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
            case shadow_point:
            case cross_point:
            case new_unknown: {
                break;
            }
        }
    }
}

auto SegmentTree::delete_last_segment() -> void {}

auto SegmentTree::add_segment(segment_info_t segment, display_state_t display_state)
    -> segment_index_t {
    const auto new_index = get_next_index();

    segments_.push_back(segment);
    display_states_.push_back(display_state);
    register_segment(new_index);

    return new_index;
}

auto SegmentTree::swap_and_delete_segment(segment_index_t index) -> void {
    if (segments_.empty()) [[unlikely]] {
        throw_exception("Cannot delete from empty segment tree.");
    }

    const auto last_index = this->last_index();
    segments_.at(index.value) = segments_.at(last_index.value);
    display_states_.at(index.value) = display_states_.at(last_index.value);

    // delete
    unregister_segment(last_index);
    segments_.pop_back();
    display_states_.pop_back();
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
    display_states_.insert(display_states_.end(), tree.display_states_.begin(),
                           tree.display_states_.end());

    return next_index;
}

auto SegmentTree::update_segment(segment_index_t index, segment_info_t segment,
                                 display_state_t display_state) -> void {
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

auto SegmentTree::segment(std::size_t index) const -> segment_info_t {
    return segments_.at(index);
}

auto SegmentTree::segment(segment_index_t index) const -> segment_info_t {
    return segments_.at(index.value);
}

auto SegmentTree::segments() const -> std::span<const segment_info_t> {
    return segments_;
}

auto SegmentTree::display_state(segment_index_t index) const -> display_state_t {
    return display_states_.at(index.value);
}

auto SegmentTree::first_index() const noexcept -> segment_index_t {
    return segment_index_t {0};
}

auto SegmentTree::last_index() const noexcept -> segment_index_t {
    const auto result = segment_count() - std::size_t {1};
    return segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(result)};
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

auto SegmentTree::validate() const -> void {
    if (display_states_.size() != segments_.size()) [[unlikely]] {
        throw_exception("Vector sizes don't match in segment tree.");
    }
    if ((has_input_ ? 1 : 0) + output_count_ > segments_.size() + 1) [[unlikely]] {
        throw_exception("To many inputs / outputs.");
    }

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
    // TODO size of segmetns, display_state, count
}

}  // namespace logicsim