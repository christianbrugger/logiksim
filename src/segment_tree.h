#ifndef LOGIKSIM_SEGMENT_TREE_H
#define LOGIKSIM_SEGMENT_TREE_H

#include "algorithm/range.h"
#include "format/struct.h"
#include "geometry/part.h"
#include "iterator_adaptor/transform_view.h"
#include "part_selection.h"
#include "vocabulary/segment.h"
#include "vocabulary/segment_index.h"
#include "vocabulary/segment_info.h"

#include <boost/container/vector.hpp>
#include <folly/small_vector.h>

#include <compare>
#include <optional>
#include <vector>

namespace logicsim {

struct connection_count_t;
struct rect_t;

namespace segment_tree {

// size_t
using vector_size_t = segment_index_t::value_type;
using vector_policy =
    folly::small_vector_policy::policy_size_type<std::make_unsigned_t<vector_size_t>>;

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
 *     + output_count_ is the number of endpoints with SegmentPointType::output
 *     + input_position_ is the position with SegmentPointType::input
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
    [[nodiscard]] inline auto indices(element_id_t element_id) const;

    // indexing
    [[nodiscard]] auto info(segment_index_t index) const -> const segment_info_t &;
    [[nodiscard]] auto line(segment_index_t index) const -> ordered_line_t;
    [[nodiscard]] auto part(segment_index_t index) const -> part_t;

    // input & outputs
    [[nodiscard]] auto has_input() const -> bool;
    [[nodiscard]] auto input_count() const -> connection_count_t;
    [[nodiscard]] auto input_position() const -> point_t;
    [[nodiscard]] auto output_count() const -> connection_count_t;

    // modifications
    auto clear() -> void;
    auto add_segment(segment_info_t segment) -> segment_index_t;
    auto add_tree(const SegmentTree &tree) -> segment_index_t;
    auto update_segment(segment_index_t index, segment_info_t segment) -> void;
    auto copy_segment(const SegmentTree &tree, segment_index_t index) -> segment_index_t;
    auto copy_segment(const SegmentTree &tree, segment_index_t index, part_t part)
        -> segment_index_t;
    auto shrink_segment(segment_index_t index, part_t new_part) -> void;
    // swaps the merging segment with last one, merges and deletes it
    auto swap_and_merge_segment(segment_index_t index, segment_index_t index_deleted)
        -> void;
    // swaps the segment with last one and deletes it
    auto swap_and_delete_segment(segment_index_t index) -> void;

    // valid parts
    auto mark_valid(segment_index_t segment_index, part_t part) -> void;
    auto unmark_valid(segment_index_t segment_index, part_t part) -> void;
    [[nodiscard]] auto valid_parts() const -> const valid_vector_t &;
    [[nodiscard]] auto valid_parts(segment_index_t segment_index) const
        -> const PartSelection &;

    auto validate() const -> void;
    auto validate_inserted() const -> void;

   private:
    auto get_next_index() const -> segment_index_t;
    auto register_segment(segment_index_t index) -> void;
    auto unregister_segment(segment_index_t index) -> void;

   private:
    segment_vector_t segments_ {};
    valid_vector_t valid_parts_vector_ {};

    // TODO change to output_count_t && make sure uninserted segments have count 0
    vector_size_t output_count_ {0};
    std::optional<point_t> input_position_ {};
};

static_assert(sizeof(SegmentTree) == 60);  // 24 + 24 + 4 + 6 (+ 2)

/**
 * @brief: Check if segment tree is a contiguous tree.
 *
 * Returns false, if segments are overlapping, could be merged or need splitting,
 * or don't form a loop free, connected tree.
 *
 * The algorithm is O(N log N).
 */
[[nodiscard]] auto is_contiguous_tree(const SegmentTree &tree) -> bool;

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

inline auto SegmentTree::indices(element_id_t element_id) const {
    return transform_view(indices(), [element_id](segment_index_t index) -> segment_t {
        return segment_t {element_id, index};
    });
};

inline auto all_lines(const SegmentTree &segment_tree) {
    return transform_view(segment_tree, &segment_info_t::line);
}

inline auto all_valid_lines(const SegmentTree &tree, segment_index_t index) {
    tree.validate();
    const auto line = tree.line(index);

    return transform_view(tree.valid_parts(index), [line](part_t part) -> ordered_line_t {
        return to_line(line, part);
    });
}

}  // namespace logicsim

#endif
