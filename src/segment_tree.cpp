#include "segment_tree.h"

#include "algorithm/accumulate.h"
#include "algorithm/transform_to_vector.h"
#include "allocated_size/folly_small_vector.h"
#include "container/graph/adjacency_graph.h"
#include "container/graph/depth_first_search.h"
#include "container/graph/visitor/empty_visitor.h"
#include "format/container.h"
#include "geometry/segment_info.h"
#include "tree_validation.h"  // TODO remove
#include "vocabulary/connection_count.h"
#include "vocabulary/part_copy_definition.h"
#include "vocabulary/rect.h"

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/zip.hpp>

namespace logicsim {

namespace segment_tree {

namespace {

auto input_position(const segment_vector_t& segments) -> std::optional<point_t> {
    auto result = std::optional<point_t> {};

    const auto set_result = [&](point_t point) {
        if (result.has_value()) [[unlikely]] {
            throw std::runtime_error("found more than one input");
        }
        result = point;
    };

    for (const auto& info : segments) {
        for (auto&& [point, type] : to_point_and_type(info)) {
            if (type == SegmentPointType::input) {
                set_result(point);
            }
        }
    }

    return result;
}

auto count_point_type(const segment_vector_t& segments, SegmentPointType type)
    -> vector_size_t {
    const auto count_type = [type](const segment_info_t& info) -> vector_size_t {
        return (info.p0_type == type ? vector_size_t {1} : vector_size_t {0}) +
               (info.p1_type == type ? vector_size_t {1} : vector_size_t {0});
    };
    return accumulate(segments, vector_size_t {0}, count_type);
}

auto output_count(const segment_vector_t& segments) -> vector_size_t {
    return count_point_type(segments, SegmentPointType::output);
}

auto all_valid_parts_within_lines(const segment_vector_t& segments,
                                  const valid_vector_t& valid_parts) -> bool {
    Expects(segments.size() == valid_parts.size());

    const auto selection_within_line = [&](std::size_t index) {
        const auto& selection = valid_parts.at(index);
        const auto& line = segments.at(index).line;
        return selection.max_offset() <= to_part(line).end;
    };

    return std::ranges::all_of(range(segments.size()), selection_within_line);
}

}  // namespace

}  // namespace segment_tree

//
// Segment Tree
//

auto SegmentTree::allocated_size() const -> std::size_t {
    return get_allocated_size(segments_) +  //
           get_allocated_size(valid_parts_vector_);
}

auto SegmentTree::begin() const -> iterator {
    return segments_.begin();
}

auto SegmentTree::end() const -> iterator {
    return segments_.end();
}

auto SegmentTree::data() const -> const segment_info_t* {
    return segments_.data();
}

auto SegmentTree::segments() const -> const segment_vector_t& {
    return segments_;
}

namespace segment_tree {

namespace {

auto sort_segments(segment_vector_t& segments, valid_vector_t& valid_parts_vector)
    -> void {
    // we sort by line
    const auto vectors = ranges::zip_view(segments, valid_parts_vector);

    const auto proj =
        [](const std::tuple<segment_info_t, PartSelection>& tuple) -> ordered_line_t {
        return std::get<segment_info_t>(tuple).line;
    };
    ranges::sort(vectors, {}, proj);
}

auto sort_point_types(segment_vector_t& segments) -> void {
    // we wrap the data so we can order it without changing the data itself
    using wrapped = std::pair<point_t, std::reference_wrapper<SegmentPointType>>;
    std::vector<wrapped> wrapped_data;

    std::ranges::transform(segments, std::back_inserter(wrapped_data),
                           [](segment_info_t& info) {
                               return wrapped {info.line.p0, info.p0_type};
                           });
    std::ranges::transform(segments, std::back_inserter(wrapped_data),
                           [](segment_info_t& info) {
                               return wrapped {info.line.p1, info.p1_type};
                           });

    // the direct view is able to change the segment points itself
    // we use a zip_view, as only it works with sorting
    const auto direct_view = ranges::zip_view(
        ranges::views::keys(wrapped_data),
        ranges::views::transform(
            wrapped_data, [](wrapped pair) -> SegmentPointType& { return pair.second; }));

    // sort first by points only
    std::ranges::sort(wrapped_data, {}, &wrapped::first);
    // sort the SegmentPointTypes for equal points
    ranges::sort(direct_view);
}

}  // namespace

}  // namespace segment_tree

auto SegmentTree::normalize() -> void {
    segment_tree::sort_segments(segments_, valid_parts_vector_);
    segment_tree::sort_point_types(segments_);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::get_next_index() const -> segment_index_t {
    return segment_index_t {gsl::narrow<segment_index_t::value_type>(segments_.size())};
}

auto SegmentTree::register_segment(segment_index_t index) -> void {
    for (auto&& [point, type] : to_point_and_type(info(index))) {
        if (type == SegmentPointType::input) {
            if (input_position_) [[unlikely]] {
                throw std::runtime_error("Segment tree already has one input.");
            }
            input_position_ = point;
        }

        else if (type == SegmentPointType::output) {
            ++output_count_;
        }
    }
}

auto SegmentTree::unregister_segment(segment_index_t index) -> void {
    for (auto&& [point, type] : to_point_and_type(info(index))) {
        if (type == SegmentPointType::input) {
            if (point != input_position_) [[unlikely]] {
                throw std::runtime_error("Tree should have input that's not present.");
            }
            input_position_.reset();
        }

        else if (type == SegmentPointType::output) {
            if (output_count_ <= 0) [[unlikely]] {
                throw std::runtime_error("Tree should have output that's not present.");
            }
            --output_count_;
        }
    }
}

auto SegmentTree::clear() -> void {
    *this = SegmentTree {};

    assert(segments_.size() == 0);
    assert(valid_parts_vector_.size() == 0);
    assert(output_count_ == 0);
    assert(input_position_ == std::nullopt);
}

auto SegmentTree::add_segment(segment_info_t segment) -> segment_index_t {
    const auto new_index = get_next_index();

    segments_.push_back(segment);
    valid_parts_vector_.emplace_back();
    register_segment(new_index);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));

    return new_index;
}

auto SegmentTree::add_tree(const SegmentTree& tree) -> segment_index_t {
    const auto next_index = get_next_index();

    if (tree.input_position_) {
        if (input_position_) [[unlikely]] {
            throw std::runtime_error("Merged tree cannot have two inputs");
        }
        input_position_ = tree.input_position_;
    }

    output_count_ += tree.output_count_;
    segments_.insert(segments_.end(), tree.segments_.begin(), tree.segments_.end());
    valid_parts_vector_.insert(valid_parts_vector_.end(),
                               tree.valid_parts_vector_.begin(),
                               tree.valid_parts_vector_.end());

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));

    return next_index;
}

auto SegmentTree::update_segment(segment_index_t index, segment_info_t segment) -> void {
    if (to_part(segment.line) != part(index)) [[unlikely]] {
        throw std::runtime_error("line length needs to stay the same");
    }

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = segment;
    register_segment(index);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::copy_segment(const SegmentTree& tree, segment_index_t index)
    -> segment_index_t {
    const auto new_index = add_segment(tree.info(index));
    valid_parts_vector_.at(new_index.value) = tree.valid_parts_vector_.at(index.value);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));

    return new_index;
}

auto SegmentTree::copy_segment(const SegmentTree& tree, segment_index_t index,
                               part_t part) -> segment_index_t {
    if (part.end > tree.part(index).end) [[unlikely]] {
        throw std::runtime_error("cannot copy part outside of line");
    }

    const auto new_info = adjust(tree.info(index), part);
    const auto new_index = add_segment(new_info);

    const auto copy_definition = part_copy_definition_t {
        .destination = to_part(new_info.line),
        .source = part,
    };
    valid_parts_vector_.at(new_index.value) =
        copy_parts(tree.valid_parts_vector_.at(index.value), copy_definition);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));

    return new_index;
}

auto SegmentTree::shrink_segment(segment_index_t index, part_t new_part) -> void {
    if (new_part.end > part(index).end) [[unlikely]] {
        throw std::runtime_error("new part cannot be outside of existing line");
    }

    const auto new_info = adjust(info(index), new_part);

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = new_info;
    register_segment(index);

    // valid parts
    const auto copy_definition = part_copy_definition_t {
        .destination = to_part(new_info.line),
        .source = new_part,
    };
    valid_parts_vector_.at(index.value) =
        copy_parts(valid_parts_vector_.at(index.value), copy_definition);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

namespace segment_tree {

namespace {

struct merged_segment_result {
    segment_info_t segment_info;
    PartSelection valid_parts;
};

auto merged_segment(const SegmentTree& tree, segment_index_t index,
                    segment_index_t index_deleted) {
    const auto& info_orig = tree.info(index);
    const auto& info_delete = tree.info(index_deleted);
    const auto info_merged = merge_touching(info_orig, info_delete);

    // valid parts
    auto entries_new = PartSelection {};
    entries_new.copy_parts(tree.valid_parts(index),
                           part_copy_definition_t {
                               .destination = to_part(info_merged.line, info_orig.line),
                               .source = to_part(info_orig.line),
                           });
    entries_new.copy_parts(tree.valid_parts(index_deleted),
                           part_copy_definition_t {
                               .destination = to_part(info_merged.line, info_delete.line),
                               .source = to_part(info_delete.line),
                           });

    return merged_segment_result {
        .segment_info = info_merged,
        .valid_parts = std::move(entries_new),
    };
}

}  // namespace

}  // namespace segment_tree

auto SegmentTree::swap_and_merge_segment(segment_index_t index,
                                         segment_index_t index_deleted) -> void {
    if (index >= index_deleted) [[unlikely]] {
        throw std::runtime_error(
            "index needs to be smaller then index_deleted, otherwise the index would "
            "change after deletion");
    }

    auto merged = segment_tree::merged_segment(*this, index, index_deleted);

    // first delete, so input count stays in bounds
    swap_and_delete_segment(index_deleted);

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = merged.segment_info;
    register_segment(index);
    // move after deletion, so class invariant is not broken for delete
    valid_parts_vector_.at(index.value) = std::move(merged.valid_parts);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::swap_and_delete_segment(segment_index_t index) -> void {
    const auto last_index = this->last_index();
    unregister_segment(index);

    // move
    if (index != last_index) {
        segments_.at(index.value) = segments_.at(last_index.value);
        valid_parts_vector_.at(index.value) =
            std::move(valid_parts_vector_.at(last_index.value));
    }

    // delete
    segments_.pop_back();
    valid_parts_vector_.pop_back();

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::empty() const noexcept -> bool {
    return segments_.empty();
}

auto SegmentTree::size() const noexcept -> std::size_t {
    return segments_.size();
}

auto SegmentTree::info(segment_index_t index) const -> const segment_info_t& {
    return segments_.at(index.value);
}

auto SegmentTree::line(segment_index_t index) const -> ordered_line_t {
    return info(index).line;
}

auto SegmentTree::part(segment_index_t index) const -> part_t {
    return to_part(line(index));
}

auto SegmentTree::mark_valid(segment_index_t segment_index, part_t marked_part) -> void {
    if (marked_part.end > part(segment_index).end) [[unlikely]] {
        throw std::runtime_error("cannot mark outside of line");
    }

    auto& valid_parts = valid_parts_vector_.at(segment_index.value);
    valid_parts.add_part(marked_part);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::unmark_valid(segment_index_t segment_index, part_t unmarked_part)
    -> void {
    if (unmarked_part.end > part(segment_index).end) [[unlikely]] {
        throw std::runtime_error("cannot unmark outside of line");
    }

    auto& valid_parts = valid_parts_vector_.at(segment_index.value);
    valid_parts.remove_part(unmarked_part);

    // post-conditions
    Ensures(segments_.size() == valid_parts_vector_.size());
    assert(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    assert(input_position_ == segment_tree::input_position(segments_));
    assert(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::valid_parts() const -> const valid_vector_t& {
    return valid_parts_vector_;
}

auto SegmentTree::valid_parts(segment_index_t segment_index) const
    -> const PartSelection& {
    return valid_parts_vector_.at(segment_index.value);
}

auto SegmentTree::first_index() const -> segment_index_t {
    return segment_index_t {0};
}

auto SegmentTree::last_index() const -> segment_index_t {
    if (empty()) [[unlikely]] {
        throw std::runtime_error("empty segment tree has no last index");
    }
    const auto result = size() - std::size_t {1};
    return segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(result)};
}

auto SegmentTree::indices() const -> forward_range_t<segment_index_t> {
    const auto count =
        segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(size())};
    return range(count);
}

auto SegmentTree::has_input() const -> bool {
    return input_position_.has_value();
}

auto SegmentTree::input_count() const -> connection_count_t {
    return input_position_ ? connection_count_t {1} : connection_count_t {0};
}

auto SegmentTree::input_position() const -> point_t {
    return input_position_.value();
}

auto SegmentTree::output_count() const -> connection_count_t {
    return connection_count_t {output_count_};
}

auto SegmentTree::format() const -> std::string {
    return fmt::format("SegmentTree({}x{}, {}, valid {})", input_count(), output_count(),
                       segments_, valid_parts_vector_);
}

auto SegmentTree::validate() const -> void {
    Expects(segments_.size() == valid_parts_vector_.size());
    Expects(segment_tree::all_valid_parts_within_lines(segments_, valid_parts_vector_));
    Expects(input_position_ == segment_tree::input_position(segments_));
    Expects(output_count_ == segment_tree::output_count(segments_));
}

auto SegmentTree::validate_inserted() const -> void {
    validate();
    Expects(is_contiguous_tree(*this));
}

//
// Free functions
//

auto calculate_normal_lines(const SegmentTree& tree) -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto index : tree.indices()) {
        const auto line = tree.line(index);
        const auto normal_parts =
            tree.valid_parts(index).inverted_selection(to_part(line));

        // convert to lines
        std::ranges::transform(
            normal_parts, std::back_inserter(result),
            [line](part_t part) -> ordered_line_t { return to_line(line, part); });
    }
    return result;
}

auto calculate_connected_segments_mask(const SegmentTree& tree, point_t p0)
    -> boost::container::vector<bool> {
    const auto graph = AdjacencyGraph<SegmentTree::vector_size_t> {all_lines(tree)};
    const auto result =
        depth_first_search_visited(graph, EmptyVisitor {}, graph.to_index(p0).value());

    if (result.status == DFSStatus::unfinished_loop) [[unlikely]] {
        throw std::runtime_error("found an unexpected loop");
    }

    // create segment mask
    auto mask = boost::container::vector<bool>(tree.size(), false);
    for (const auto segment_index : tree.indices()) {
        const auto line = tree.line(segment_index);

        const auto p0_index = graph.to_index(line.p0).value();
        mask[segment_index.value] = result.visited[p0_index];
    }

    return mask;
}

auto is_contiguous_tree(const SegmentTree& tree) -> bool {
    return segments_are_contiguous_tree(transform_to_vector(all_lines(tree)));
}

auto calculate_bounding_rect(const SegmentTree& tree) -> rect_t {
    if (tree.empty()) [[unlikely]] {
        throw std::runtime_error("empty segment tree has no bounding-rect");
    }

    auto p_min = point_t {grid_t::max(), grid_t::max()};
    auto p_max = point_t {grid_t::min(), grid_t::min()};

    for (const auto& line : all_lines(tree)) {
        p_min.x = std::min(line.p0.x, p_min.x);
        p_min.y = std::min(line.p0.y, p_min.y);

        p_max.x = std::max(line.p1.x, p_max.x);
        p_max.y = std::max(line.p1.y, p_max.y);
    }
    return rect_t {p_min, p_max};
}

}  // namespace logicsim
