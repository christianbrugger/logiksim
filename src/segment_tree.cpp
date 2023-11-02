#include "segment_tree.h"

#include "algorithm/accumulate.h"
#include "algorithm/range.h"
#include "algorithm/transform_if.h"
#include "algorithm/transform_to_vector.h"
#include "allocated_size/folly_small_vector.h"
#include "allocated_size/trait.h"
#include "container/graph/adjacency_graph.h"
#include "container/graph/depth_first_search.h"
#include "container/graph/visitor/empty_visitor.h"
#include "exception.h"
#include "format/container.h"
#include "geometry/line.h"
#include "geometry/segment_info.h"
#include "line_tree.h"
#include "logging.h"
#include "tree_validation.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/part_copy_definition.h"
#include "vocabulary/rect.h"

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/zip.hpp>

namespace logicsim {

//
// Segment Tree
//

auto SegmentTree::clear() -> void {
    *this = SegmentTree {};
}

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

auto SegmentTree::normalize() -> void {
    sort_segments();
    sort_point_types();
}

auto SegmentTree::sort_segments() -> void {
    // we sort by line
    const auto vectors = ranges::zip_view(segments_, valid_parts_vector_);

    const auto proj =
        [](const std::tuple<segment_info_t, PartSelection>& tuple) -> ordered_line_t {
        return std::get<segment_info_t>(tuple).line;
    };
    ranges::sort(vectors, {}, proj);
}

auto SegmentTree::sort_point_types() -> void {
    // we wrap the data so we can order it without changing the data itself
    using wrapped = std::pair<point_t, std::reference_wrapper<SegmentPointType>>;
    std::vector<wrapped> wrapped_data;

    std::ranges::transform(segments_, std::back_inserter(wrapped_data),
                           [](segment_info_t& info) {
                               return wrapped {info.line.p0, info.p0_type};
                           });
    std::ranges::transform(segments_, std::back_inserter(wrapped_data),
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
                if (input_position_) [[unlikely]] {
                    throw_exception("Segment tree already has one input.");
                }
                input_position_ = point;
                break;
            }

            case output: {
                ++output_count_;
                break;
            }

            case corner_point:
            case cross_point:
            case shadow_point:
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
                if (point != input_position_) [[unlikely]] {
                    throw_exception("Tree should have input that's not present.");
                }
                input_position_.reset();
                break;
            }

            case output: {
                if (output_count_ <= 0) [[unlikely]] {
                    throw_exception("Tree should have output thats not present.");
                }
                --output_count_;
                break;
            }

            case corner_point:
            case cross_point:
            case shadow_point:
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
        using std::swap;
        swap(valid_parts_vector_.at(index.value),
             valid_parts_vector_.at(last_index.value));
    }

    // delete
    segments_.pop_back();
    valid_parts_vector_.pop_back();
}

auto SegmentTree::add_tree(const SegmentTree& tree) -> segment_index_t {
    const auto next_index = get_next_index();

    if (tree.input_position_) {
        if (input_position_) [[unlikely]] {
            throw_exception("Merged tree cannot have two inputs");
        }
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
    valid_parts_vector_.emplace_back();
    register_segment(new_index);

    return new_index;
}

auto SegmentTree::update_segment(segment_index_t index, segment_info_t segment) -> void {
    if (distance(segment.line) != distance(line(index))) {
        throw_exception("line length needs to be the same");
    }

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = segment;
    register_segment(index);
}

auto SegmentTree::shrink_segment(segment_index_t index, part_t part) -> void {
    const auto new_info = adjust(info(index), part);

    // update segment
    unregister_segment(index);
    segments_.at(index.value) = new_info;
    register_segment(index);

    // valid parts
    const auto parts = part_copy_definition_t {
        .destination = to_part(new_info.line),
        .source = part,
    };
    valid_parts_vector_.at(index.value) =
        copy_parts(valid_parts_vector_.at(index.value), parts);
}

auto SegmentTree::swap_and_merge_segment(segment_index_t index,
                                         segment_index_t index_deleted) -> void {
    if (index >= index_deleted) [[unlikely]] {
        throw_exception(
            "index needs to be smaller then index_deleted, otherwise the index would "
            "change after deletionion");
    }

    const auto info_orig = info(index);
    const auto info_delete = info(index_deleted);
    const auto info_merged = merge_touching(info_orig, info_delete);

    // copy valid parts
    auto& entries_orig = valid_parts_vector_.at(index.value);
    auto& entries_delete = valid_parts_vector_.at(index_deleted.value);

    auto new_entries = copy_parts(
        entries_orig, part_copy_definition_t {
                          .destination = to_part(info_merged.line, info_orig.line),
                          .source = to_part(info_orig.line),
                      });
    new_entries.copy_parts(entries_delete,
                           part_copy_definition_t {
                               .destination = to_part(info_merged.line, info_delete.line),
                               .source = to_part(info_delete.line),
                           });
    using std::swap;
    swap(entries_orig, new_entries);

    // first delete, so input count stays in bounds
    swap_and_delete_segment(index_deleted);
    // update segment
    unregister_segment(index);
    segments_.at(index.value) = info_merged;
    register_segment(index);
}

auto SegmentTree::copy_segment(const SegmentTree& tree, segment_index_t index)
    -> segment_index_t {
    const auto new_index = add_segment(tree.info(index));
    valid_parts_vector_.at(new_index.value) = tree.valid_parts_vector_.at(index.value);
    return new_index;
}

auto SegmentTree::copy_segment(const SegmentTree& tree, segment_index_t index,
                               part_t part) -> segment_index_t {
    const auto new_info = adjust(tree.info(index), part);

    const auto new_index = add_segment(new_info);

    const auto parts = part_copy_definition_t {
        .destination = to_part(new_info.line),
        .source = part,
    };
    valid_parts_vector_.at(new_index.value) =
        copy_parts(tree.valid_parts_vector_.at(index.value), parts);

    return new_index;
}

auto SegmentTree::empty() const noexcept -> bool {
    return segments_.empty();
}

auto SegmentTree::size() const noexcept -> std::size_t {
    return segments_.size();
}

auto SegmentTree::info(segment_index_t index) const -> segment_info_t {
    return segments_.at(index.value);
}

auto SegmentTree::line(segment_index_t index) const -> ordered_line_t {
    return info(index).line;
}

auto SegmentTree::part(segment_index_t index) const -> part_t {
    return to_part(line(index));
}

auto SegmentTree::mark_valid(segment_index_t segment_index, part_t part) -> void {
    auto& valid_parts = valid_parts_vector_.at(segment_index.value);
    valid_parts.add_part(part);
}

auto SegmentTree::unmark_valid(segment_index_t segment_index, part_t part) -> void {
    auto& valid_parts = valid_parts_vector_.at(segment_index.value);
    valid_parts.remove_part(part);
}

auto SegmentTree::valid_parts() const -> const valid_vector_t& {
    return valid_parts_vector_;
}

auto SegmentTree::valid_parts(segment_index_t segment_index) const
    -> const PartSelection& {
    return valid_parts_vector_.at(segment_index.value);
}

auto SegmentTree::first_index() const noexcept -> segment_index_t {
    return segment_index_t {0};
}

auto SegmentTree::last_index() const noexcept -> segment_index_t {
    if (empty()) [[unlikely]] {
        throw_exception("empty segment tree has no last index");
    }
    const auto result = size() - std::size_t {1};
    return segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(result)};
}

auto SegmentTree::indices() const noexcept -> forward_range_t<segment_index_t> {
    const auto count =
        segment_index_t {gsl::narrow_cast<segment_index_t::value_type>(size())};
    return range(count);
}

auto SegmentTree::has_input() const noexcept -> bool {
    return input_position_.has_value();
}

auto SegmentTree::input_count() const noexcept -> connection_count_t {
    return input_position_ ? connection_count_t {1} : connection_count_t {0};
}

auto SegmentTree::input_position() const -> point_t {
    return input_position_.value();
}

auto SegmentTree::output_count() const noexcept -> connection_count_t {
    return connection_count_t {output_count_};
}

auto SegmentTree::format() const -> std::string {
    return fmt::format("SegmentTree({}x{}, {}, valid {})", input_count(), output_count(),
                       segments_, valid_parts_vector_);
}

auto recalculate_first_input_position(const SegmentTree& tree) -> std::optional<point_t> {
    for (const auto& info : tree) {
        if (info.p0_type == SegmentPointType::input) {
            return info.line.p0;
        } else if (info.p1_type == SegmentPointType::input) {
            return info.line.p1;
        }
    }

    return std::nullopt;
}

auto count_point_type(const SegmentTree& tree, SegmentPointType type)
    -> connection_count_t {
    const auto proj = [&](segment_info_t info) -> connection_count_t {
        return (info.p0_type == type ? connection_count_t {1} : connection_count_t {0}) +
               (info.p1_type == type ? connection_count_t {1} : connection_count_t {0});
    };
    return accumulate(transform_view(tree, proj), connection_count_t {0});
}

auto validate_output_count(const SegmentTree& tree) -> void {
    if (tree.output_count() != count_point_type(tree, SegmentPointType::output))
        [[unlikely]] {
        throw_exception("Tree input output count is wrong");
    }
}

auto validate_input_count_and_position(const SegmentTree& tree) -> void {
    if (tree.input_count() != count_point_type(tree, SegmentPointType::input))
        [[unlikely]] {
        throw_exception("Tree input count is wrong");
    }
    if (tree.has_input()) {
        const auto input = recalculate_first_input_position(tree);
        if (!input || input.value() != tree.input_position()) [[unlikely]] {
            throw_exception("Tree has stored the wrong input");
        }
    }
}

auto SegmentTree::validate() const -> void {
    if (valid_parts_vector_.size() != segments_.size()) [[unlikely]] {
        throw_exception("Vector sizes don't match in segment tree.");
    }

    // input / output count
    validate_input_count_and_position(*this);
    validate_output_count(*this);

    // valid parts
    const auto is_part_of_line = [&](segment_index_t index) -> bool {
        return valid_parts(index).max_offset() <= to_part(line(index)).end;
    };
    if (!std::ranges::all_of(indices(), is_part_of_line)) {
        throw std::runtime_error("part is not part of line");
    }
}

auto SegmentTree::validate_inserted() const -> void {
    validate();

    if (!segments_are_normalized_tree(
            transform_to_vector(all_lines(*this)))) {
        throw std::runtime_error("segments are not a normalized tree");
    }
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
        throw_exception("found an unexpected loop");
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

auto calculate_bounding_rect(const SegmentTree& tree) -> rect_t {
    if (tree.empty()) [[unlikely]] {
        throw_exception("empty segment tree has no bounding-rect");
    }

    auto p_min = point_t {grid_t::max(), grid_t::max()};
    auto p_max = point_t {grid_t::min(), grid_t::min()};

    for (const auto& line : all_lines(tree)) {
        p_min.x = std::min(line.p0.x, p_min.x);
        p_min.y = std::min(line.p0.y, p_min.y);

        p_max.x = std::min(line.p1.x, p_max.x);
        p_max.y = std::min(line.p1.y, p_max.y);
    }
    return rect_t {p_min, p_max};
}

}  // namespace logicsim
