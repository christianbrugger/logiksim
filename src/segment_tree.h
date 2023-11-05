#ifndef LOGIKSIM_SEGMENT_TREE_H
#define LOGIKSIM_SEGMENT_TREE_H

#include "algorithm/range.h"
#include "format/struct.h"
#include "geometry/part.h"
#include "iterator_adaptor/transform_view.h"
#include "part_selection.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/segment.h"
#include "vocabulary/segment_index.h"
#include "vocabulary/segment_info.h"

#include <boost/container/vector.hpp>
#include <folly/small_vector.h>

#include <compare>
#include <optional>
#include <vector>

namespace logicsim {

struct rect_t;

namespace segment_tree {

// sizes
using vector_size_t = std::make_unsigned_t<segment_index_t::value_type>;
using vector_policy = folly::small_vector_policy::policy_size_type<vector_size_t>;

// segment_vector
using segment_vector_t = folly::small_vector<segment_info_t, 2, vector_policy>;
static_assert(sizeof(segment_vector_t) == 24);

// valid_vector
using valid_vector_t = folly::small_vector<PartSelection, 2, vector_policy>;
static_assert(sizeof(valid_vector_t) == 24);

}  // namespace segment_tree

/**
 * @brief: A collection of lines with valid status.
 *
 * Note that the segment tree can never have more than one input.
 *
 * Class invariants:
 *     + size of `segments_` and `valid_parts_vector_` match
 *     + for each index valid_parts::max_offset is within the corresponding line
 *     + input_position_ is the position with SegmentPointType::input
 *     + output_count_ is the number of endpoints with SegmentPointType::output
 */
class SegmentTree {
   public:
    using segment_vector_t = segment_tree::segment_vector_t;
    using valid_vector_t = segment_tree::valid_vector_t;
    using vector_size_t = segment_tree::vector_size_t;

    using value_type = segment_info_t;
    using iterator = segment_vector_t::const_iterator;

   public:
    [[nodiscard]] auto format() const -> std::string;
    /**
     * @brief: brings the tree into its canonical,
     *         so that visual equivalent trees compare equal
     */
    auto normalize() -> void;
    [[nodiscard]] auto operator==(const SegmentTree &) const -> bool = default;
    [[nodiscard]] auto operator<=>(const SegmentTree &) const = default;

    // size
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    // iterators
    [[nodiscard]] auto begin() const -> iterator;
    [[nodiscard]] auto end() const -> iterator;
    [[nodiscard]] auto data() const -> const segment_info_t *;
    [[nodiscard]] auto segments() const -> const segment_vector_t &;

    // indices
    [[nodiscard]] auto first_index() const -> segment_index_t;
    [[nodiscard]] auto last_index() const -> segment_index_t;
    [[nodiscard]] auto indices() const -> forward_range_t<segment_index_t>;
    [[nodiscard]] inline auto indices(wire_id_t wire_id) const;

    // indexing
    [[nodiscard]] auto info(segment_index_t index) const -> const segment_info_t &;
    [[nodiscard]] auto line(segment_index_t index) const -> ordered_line_t;
    [[nodiscard]] auto part(segment_index_t index) const -> part_t;

    // input & outputs
    [[nodiscard]] auto has_input() const -> bool;
    [[nodiscard]] auto input_count() const -> connection_count_t;
    [[nodiscard]] auto input_position() const -> point_t;
    [[nodiscard]] auto output_count() const -> connection_count_t;

    //
    // modifications
    //

    auto clear() -> void;

    /**
     * @brief: Add a new segment to the tree.
     *
     * Throws if number of inputs exceeds one after adding the segment.
     *
     * Returns the new segment index.
     */
    auto add_segment(segment_info_t segment) -> segment_index_t;

    /**
     * @brief: Add segments of the given tree to this tree including valid parts.
     *
     * Throws if both trees have an input.
     *
     * Returns the first index of the added segments. Segments added until last_index.
     */
    auto add_tree(const SegmentTree &tree) -> segment_index_t;

    /**
     * @brief: Update line position, orientation and endpoints.
     *
     * Throws if the line length is different.
     */
    auto update_segment(segment_index_t index, segment_info_t segment) -> void;

    /**
     * @brief: Copy the given full segment into this tree including valid parts.
     *
     * Throws if number of inputs would exceed one.
     *
     * Returns segment index of the added entry.
     */
    auto copy_segment(const SegmentTree &tree, segment_index_t index) -> segment_index_t;

    /**
     * @brief: Copy the a sub-part of the given segment to this tree.
     *
     * Throws if number of inputs would exceed one.
     *
     * Returns segment index of the added entry.
     */
    auto copy_segment(const SegmentTree &tree, segment_index_t index, part_t part)
        -> segment_index_t;

    /**
     * @brief: Shrinks the specified segment to the new part.
     *
     * Throws if the part is outside the line.
     *
     * Note part can have arbitrary start and end offsets,
     * e.g. part [4, 8] for line [(0, 0), (10, 0)].
     *
     * Endpoints are set to `SegmentPointType::shadow_point` if they are not included.
     */
    auto shrink_segment(segment_index_t index, part_t new_part) -> void;

    struct merge_definition_t {
        segment_index_t index_merge_to;
        segment_index_t index_deleted;
    };
    /**
     * @brief: Merge two touching segments and delete the second.
     *
     * Throws if segments are not touching at an endpoint or are not parallel.
     * Throws if index is larger than index_deleted, as this would change the index
     *         after deletion.
     *
     * Note endpoint types at the merge-point are discarded.
     * Note the deleted segment is swapped with the last element and then merged.
     */
    auto swap_and_merge_segment(merge_definition_t definition) -> void;

    /**
     * @brief: Delete the given segment.
     *
     * Note the last segment is swapped in place of the deleted segment.
     */
    auto swap_and_delete_segment(segment_index_t index) -> void;

    // valid parts
    auto mark_valid(segment_index_t segment_index, part_t marked_part) -> void;
    auto unmark_valid(segment_index_t segment_index, part_t unmarked_part) -> void;
    [[nodiscard]] auto valid_parts() const -> const valid_vector_t &;
    [[nodiscard]] auto valid_parts(segment_index_t segment_index) const
        -> const PartSelection &;

   private:
    auto get_next_index() const -> segment_index_t;
    auto register_segment(segment_index_t index) -> void;
    auto unregister_segment(segment_index_t index) -> void;

   private:
    segment_vector_t segments_ {};
    valid_vector_t valid_parts_vector_ {};

    std::optional<point_t> input_position_ {};
    connection_count_t output_count_ {0};
};

static_assert(sizeof(SegmentTree) == 56);  // 24 + 24 + 6 + 2

[[nodiscard]] auto calculate_bounding_rect(const SegmentTree &tree) -> rect_t;

[[nodiscard]] inline auto all_lines(const SegmentTree &tree);

[[nodiscard]] inline auto all_valid_lines(const SegmentTree &tree, segment_index_t index);

[[nodiscard]] auto calculate_normal_lines(const SegmentTree &tree)
    -> std::vector<ordered_line_t>;

[[nodiscard]] auto calculate_connected_segments_mask(const SegmentTree &tree, point_t p0)
    -> boost::container::vector<bool>;

//
// Implementation
//

inline auto SegmentTree::indices(wire_id_t wire_id) const {
    return transform_view(indices(), [wire_id](segment_index_t index) -> segment_t {
        return segment_t {wire_id, index};
    });
};

inline auto all_lines(const SegmentTree &segment_tree) {
    return transform_view(segment_tree, &segment_info_t::line);
}

inline auto all_valid_lines(const SegmentTree &tree, segment_index_t index) {
    const auto line = tree.line(index);

    return transform_view(tree.valid_parts(index), [line](part_t part) -> ordered_line_t {
        return to_line(line, part);
    });
}

}  // namespace logicsim

#endif
