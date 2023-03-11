#ifndef LOGIKSIM_SEGMENT_TREE_H
#define LOGIKSIM_SEGMENT_TREE_H

#include "format.h"
#include "vocabulary.h"

#include <folly/small_vector.h>

#include <optional>
#include <span>
#include <vector>

namespace logicsim {

struct SegmentInfo {
    line_t line {};
    bool p0_is_input {false};
    bool p1_is_output {false};
};

class SegmentTree {
   public:
    using index_t = uint16_t;

   public:
    [[nodiscard]] constexpr SegmentTree() = default;
    [[nodiscard]] explicit SegmentTree(SegmentInfo segment);

    auto swap(SegmentTree &other) noexcept -> void;

    auto add_segment(SegmentInfo segment) -> void;
    auto add_tree(const SegmentTree &tree) -> void;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto segment_count() const noexcept -> std::size_t;
    [[nodiscard]] auto segment(std::size_t index) const -> line_t;
    [[nodiscard]] auto segments() const -> std::span<const line_t>;

    [[nodiscard]] auto has_input() const noexcept -> bool;
    [[nodiscard]] auto input_position() const -> point_t;
    //[[nodiscard]] auto input_orientation() const -> orientation_t;

    [[nodiscard]] auto cross_points() const -> std::span<const point_t>;

    [[nodiscard]] auto output_count() const noexcept -> std::size_t;
    [[nodiscard]] auto output_positions() const -> std::span<const point_t>;
    [[nodiscard]] auto output_position(std::size_t index) const -> point_t;
    //[[nodiscard]] auto output_orientation(std::size_t index) const -> orientation_t;

    [[nodiscard]] auto format() const -> std::string;
    auto verify() const -> void;

   private:
    using policy = folly::small_vector_policy::policy_size_type<index_t>;
    using segment_vector_t = folly::small_vector<line_t, 2, policy>;
    using point_vector_t = folly::small_vector<point_t, 2, policy>;
    // using output_orientation_vector_t = folly::small_vector<orientation_t, 8, policy>;

    static_assert(sizeof(segment_vector_t) == 18);
    static_assert(sizeof(point_vector_t) == 10);
    // static_assert(sizeof(output_orientation_vector_t) == 10);

    segment_vector_t segments_ {};
    point_vector_t cross_points_ {};
    point_vector_t output_positions_ {};
    // output_orientation_vector_t output_orientations_ {};

    // orientation_t input_orientation_ {orientation_t::undirected};
    point_t input_position_ {};
    bool has_input_ {false};
};

static_assert(sizeof(SegmentTree) == 44);  // 18 + 10 + 10 + 4 + 1 (+ 1)

auto swap(SegmentTree &a, SegmentTree &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::SegmentTree &a, logicsim::SegmentTree &b) noexcept -> void;

#endif
