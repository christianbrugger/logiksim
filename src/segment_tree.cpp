#include "segment_tree.h"

#include "line_tree.h"
#include "range.h"

#include <range/v3/view/map.hpp>
#include <range/v3/view/zip.hpp>

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

auto is_connection(SegmentPointType point_type) -> bool {
    using enum SegmentPointType;
    return point_type == input || point_type == output;
}

auto order_points(segment_info_t a, segment_info_t b)
    -> std::tuple<segment_info_t, segment_info_t> {
    if (a.line <= b.line) {
        return std::make_tuple(a, b);
    }
    return std::make_tuple(b, a);
}

auto adjust(const segment_info_t segment_info, const part_t part) -> segment_info_t {
    const auto new_line = to_line(segment_info.line, part);

    const auto p0_changed = new_line.p0 != segment_info.line.p0;
    const auto p1_changed = new_line.p1 != segment_info.line.p1;

    return segment_info_t {
        .line = new_line,

        .p0_type = p0_changed ? SegmentPointType::shadow_point : segment_info.p0_type,
        .p1_type = p1_changed ? SegmentPointType::shadow_point : segment_info.p1_type,

        .p0_connection_id = p0_changed ? null_connection : segment_info.p0_connection_id,
        .p1_connection_id = p1_changed ? null_connection : segment_info.p1_connection_id,
    };
}

auto merge_touching(const segment_info_t segment_info_0,
                    const segment_info_t segment_info_1) -> segment_info_t {
    const auto [a, b] = order_points(segment_info_0, segment_info_1);

    if (a.line.p1 != b.line.p0) [[unlikely]] {
        throw_exception("segments need to have common shared point");
    }

    return segment_info_t {
        .line = ordered_line_t {a.line.p0, b.line.p1},

        .p0_type = a.p0_type,
        .p1_type = b.p1_type,

        .p0_connection_id = a.p0_connection_id,
        .p1_connection_id = b.p1_connection_id,
    };
}

auto segment_info_t::format() const -> std::string {
    const auto connection_string_0
        = p0_connection_id ? p0_connection_id.format() + " " : "";
    const auto connection_string_1
        = p1_connection_id ? " " + p1_connection_id.format() : "";

    return fmt::format("Segment({}{} {} - {} {}{})", connection_string_0, p0_type,
                       line.p0, line.p1, p1_type, connection_string_1);
}

//
// Segment Tree
//

auto SegmentTree::clear() -> void {
    *this = SegmentTree {};
}

auto SegmentTree::swap(SegmentTree& other) noexcept -> void {
    using std::swap;

    segments_.swap(other.segments_);
    valid_parts_vector_.swap(other.valid_parts_vector_);

    swap(output_count_, other.output_count_);
    swap(input_position_, other.input_position_);
    swap(has_input_, other.has_input_);
}

auto SegmentTree::normalize() -> void {
    sort_segments();
    sort_point_types();
}

auto SegmentTree::sort_segments() -> void {
    // we sort by ordered line
    const auto vectors = ranges::zip_view(segments_, valid_parts_vector_);

    const auto proj
        = [](std::tuple<segment_info_t, parts_vector_t> tuple) -> ordered_line_t {
        return std::get<segment_info_t>(tuple).line;
    };
    std::ranges::sort(vectors, {}, proj);
}

auto SegmentTree::sort_point_types() -> void {
    // first we sort by points only
    using wrapped
        = std::pair<point_t, std::pair<std::reference_wrapper<SegmentPointType>,
                                       std::reference_wrapper<connection_id_t>>>;
    std::vector<wrapped> refs;

    std::ranges::transform(segments_, std::back_inserter(refs), [](segment_info_t& info) {
        return std::pair {info.line.p0, std::pair {std::ref(info.p0_type),
                                                   std::ref(info.p0_connection_id)}};
    });
    std::ranges::transform(segments_, std::back_inserter(refs), [](segment_info_t& info) {
        return std::pair {info.line.p1, std::pair {std::ref(info.p1_type),
                                                   std::ref(info.p1_connection_id)}};
    });
    std::ranges::sort(refs, {}, &wrapped::first);

    // now we sort the SegmentPointTypes for equal points
    // to modify the original data, we unwrapp the references
    // we need a zip view, as it works std sorting
    const auto data_direct = ranges::zip_view(
        ranges::views::keys(refs),
        ranges::views::transform(
            refs, [](wrapped pair) -> SegmentPointType& { return pair.second.first; }),
        ranges::views::transform(
            refs, [](wrapped pair) -> connection_id_t& { return pair.second.second; }));

    std::ranges::sort(data_direct);
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

auto SegmentTree::swap_and_delete_segment(segment_index_t index) -> void {
    if (segments_.empty()) [[unlikely]] {
        throw_exception("Cannot delete from empty segment tree.");
    }

    const auto last_index = this->last_index();
    unregister_segment(index);

    // swap
    if (index != last_index) {
        segments_.at(index.value) = segments_.at(last_index.value);
        valid_parts_vector_.at(index.value)
            .swap(valid_parts_vector_.at(last_index.value));
    }

    // delete
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

auto SegmentTree::add_segment(segment_info_t segment) -> segment_index_t {
    const auto new_index = get_next_index();

    segments_.push_back(segment);
    valid_parts_vector_.push_back(parts_vector_t {});
    register_segment(new_index);

    return new_index;
}

auto SegmentTree::update_segment(segment_index_t index, segment_info_t segment) -> void {
    if (distance(segment.line) != distance(segment_line(index))) {
        throw_exception("line length needs to be the same");
    }

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = segment;
    register_segment(index);
}

auto SegmentTree::shrink_segment(segment_index_t index, part_t part) -> void {
    const auto new_info = adjust(segment_info(index), part);

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = new_info;
    register_segment(index);

    // valid parts
    valid_parts_vector_.at(index.value)
        = copy_parts(valid_parts_vector_.at(index.value), part);
}

auto SegmentTree::swap_and_merge_segment(segment_index_t index,
                                         segment_index_t index_deleted) -> void {
    // otherwise the index changes after deletion
    if (index >= index_deleted) [[unlikely]] {
        throw_exception("index needs to be smaller then index_deleted");
    }

    const auto info_deleted = segment_info(index_deleted);
    const auto info_merged = merge_touching(segment_info(index), info_deleted);

    // copy valid parts
    auto& source_entries = valid_parts_vector_.at(index_deleted.value);
    auto& destination_entries = valid_parts_vector_.at(index.value);
    const auto destination_part = to_part(info_merged.line, info_deleted.line);
    copy_parts(source_entries, destination_entries, destination_part);

    // first delete, so input count stays in bounds
    swap_and_delete_segment(index_deleted);
    // update segment
    unregister_segment(index);
    segments_.at(index.value) = info_merged;
    register_segment(index);
}

auto SegmentTree::copy_segment(const SegmentTree& tree, segment_index_t index)
    -> segment_index_t {
    const auto new_index = add_segment(tree.segment_info(index));
    valid_parts_vector_.at(new_index.value) = tree.valid_parts_vector_.at(index.value);
    return new_index;
}

auto SegmentTree::copy_segment(const SegmentTree& tree, segment_index_t index,
                               part_t part) -> segment_index_t {
    const auto new_info = adjust(tree.segment_info(index), part);

    const auto new_index = add_segment(new_info);
    valid_parts_vector_.at(new_index.value)
        = copy_parts(valid_parts_vector_.at(index.value), part);

    return new_index;
}

auto SegmentTree::empty() const noexcept -> bool {
    return segments_.empty();
}

auto SegmentTree::segment_count() const noexcept -> std::size_t {
    return segments_.size();
}

auto SegmentTree::segment_info(segment_index_t index) const -> segment_info_t {
    return segments_.at(index.value);
}

auto SegmentTree::segment_line(segment_index_t index) const -> ordered_line_t {
    return segment_info(index).line;
}

auto SegmentTree::segment_part(segment_index_t index) const -> part_t {
    return to_part(segment_line(index));
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
