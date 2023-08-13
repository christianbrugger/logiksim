#ifndef LOGIKSIM_SEGMENT_TREE_H
#define LOGIKSIM_SEGMENT_TREE_H

#include "format.h"
#include "geometry.h"
#include "segment_tree_type.h"
#include "vocabulary.h"

#include <boost/container/vector.hpp>
#include <folly/small_vector.h>

#include <optional>
#include <span>
#include <vector>

// Open issues:
// - How to deal with segment that connects two outputs: output < ---- > output

namespace logicsim {

class SegmentTree;

[[nodiscard]] auto order_points(segment_info_t segment0, segment_info_t segment1)
    -> std::tuple<segment_info_t, segment_info_t>;
[[nodiscard]] auto adjust(segment_info_t segment, part_t part) -> segment_info_t;
[[nodiscard]] auto merge_touching(const segment_info_t segment_info_0,
                                  const segment_info_t segment_info_1) -> segment_info_t;

[[nodiscard]] auto calculate_bounding_rect(const SegmentTree &tree) -> rect_t;

namespace detail::segment_tree {
using index_t = std::make_unsigned_t<segment_index_t::value_type>;
static_assert(sizeof(index_t) == sizeof(segment_index_t::value_type));

using policy = folly::small_vector_policy::policy_size_type<uint16_t>;  // index_t
using parts_vector_t = folly::small_vector<part_t, 2, policy>;
static_assert(sizeof(parts_vector_t) == 10);  // 2 * 4 + 2

}  // namespace detail::segment_tree

class SegmentTree {
   public:
    using index_t = detail::segment_tree::index_t;
    using parts_vector_t = detail::segment_tree::parts_vector_t;

   public:
    [[nodiscard]] constexpr SegmentTree() = default;
    auto swap(SegmentTree &other) noexcept -> void;
    auto normalize() -> void;  // bring it into a form that can be compared

    [[nodiscard]] auto operator==(const SegmentTree &) const -> bool = default;
    [[nodiscard]] auto operator<=>(const SegmentTree &) const = default;

    auto clear() -> void;
    auto add_segment(segment_info_t segment) -> segment_index_t;
    auto add_tree(const SegmentTree &tree) -> segment_index_t;
    auto update_segment(segment_index_t index, segment_info_t segment) -> void;
    auto copy_segment(const SegmentTree &tree, segment_index_t index) -> segment_index_t;
    auto copy_segment(const SegmentTree &tree, segment_index_t index, part_t part)
        -> segment_index_t;
    auto shrink_segment(segment_index_t index, part_t part) -> void;
    // swaps the merging segment with last one, merges and deletes it
    auto swap_and_merge_segment(segment_index_t index, segment_index_t index_deleted)
        -> void;
    // swaps the segment with last one and deletes it
    auto swap_and_delete_segment(segment_index_t index) -> void;

    // segments
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto segment_count() const noexcept -> std::size_t;
    [[nodiscard]] auto segment_info(segment_index_t index) const -> segment_info_t;
    [[nodiscard]] auto segment_line(segment_index_t index) const -> ordered_line_t;
    [[nodiscard]] auto segment_part(segment_index_t index) const -> part_t;
    [[nodiscard]] auto segment_infos() const -> std::span<const segment_info_t>;

    // valid parts
    auto mark_valid(segment_index_t segment_index, part_t part) -> void;
    auto unmark_valid(segment_index_t segment_index, part_t part) -> void;
    [[nodiscard]] auto valid_parts() const -> std::span<const parts_vector_t>;
    [[nodiscard]] auto valid_parts(segment_index_t segment_index) const
        -> std::span<const part_t>;

    // indices
    [[nodiscard]] auto first_index() const noexcept -> segment_index_t;
    [[nodiscard]] auto last_index() const noexcept -> segment_index_t;
    [[nodiscard]] auto indices() const noexcept -> forward_range_t<segment_index_t>;

    [[nodiscard]] auto indices(element_id_t element_id) const {
        return transform_view(indices(), [=](segment_index_t index) -> segment_t {
            return segment_t {element_id, index};
        });
    };

    // input & outputs
    [[nodiscard]] auto has_input() const noexcept -> bool;
    [[nodiscard]] auto input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto input_position() const -> point_t;
    [[nodiscard]] auto output_count() const noexcept -> std::size_t;

    [[nodiscard]] auto format() const -> std::string;
    auto validate() const -> void;
    auto validate_inserted() const -> void;

   private:
    auto get_next_index() const -> segment_index_t;
    auto register_segment(segment_index_t index) -> void;
    auto unregister_segment(segment_index_t index) -> void;
    auto sort_segments() -> void;
    auto sort_point_types() -> void;

   private:
    using policy = folly::small_vector_policy::policy_size_type<index_t>;
    using segment_vector_t = folly::small_vector<segment_info_t, 2, policy>;
    static_assert(sizeof(segment_vector_t) == 24);  // 2 * 10 + 4

    using valid_vector_t = folly::small_vector<parts_vector_t, 2, policy>;
    static_assert(sizeof(valid_vector_t) == 24);  // 2 * 10 + 4

    segment_vector_t segments_ {};
    valid_vector_t valid_parts_vector_ {};

    index_t output_count_ {0};
    // optional has bad memory size, so we do it ourselves
    point_t input_position_ {};
    bool has_input_ {false};
};

static_assert(sizeof(SegmentTree) == 60);  // 24 + 24 + 4 + 4 + 1 (+ 3)

auto swap(SegmentTree &a, SegmentTree &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::SegmentTree &a, logicsim::SegmentTree &b) noexcept -> void;

namespace logicsim {

inline auto all_lines(const SegmentTree &tree) {
    return transform_view(tree.segment_infos(),
                          [](segment_info_t info) { return info.line; });
}

inline auto all_valid_lines(const SegmentTree &tree, segment_index_t index) {
    const auto line = tree.segment_line(index);

    return transform_view(tree.valid_parts(index), [line](part_t part) -> ordered_line_t {
        return to_line(line, part);
    });
}

auto calculate_normal_parts(const SegmentTree &tree, segment_index_t index,
                            detail::segment_tree::parts_vector_t &result) -> void;

inline auto calculate_normal_lines(const SegmentTree &tree) {
    auto parts = detail::segment_tree::parts_vector_t {};
    auto result = std::vector<ordered_line_t> {};

    for (const auto index : tree.indices()) {
        // get parts
        parts.clear();
        calculate_normal_parts(tree, index, parts);

        // convert to lines
        const auto line = tree.segment_line(index);
        std::ranges::transform(
            parts, std::back_inserter(result),
            [line](part_t part) -> ordered_line_t { return to_line(line, part); });
    }
    return result;
}

auto calculate_connected_segments_mask(const SegmentTree &tree, point_t p0)
    -> boost::container::vector<bool>;
}  // namespace logicsim

#endif
