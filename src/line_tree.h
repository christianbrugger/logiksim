#ifndef LOGIKSIM_LINETREE_H
#define LOGIKSIM_LINETREE_H

#include "format.h"
#include "geometry.h"
#include "iterator_adaptor.h"

#include <fmt/core.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <functional>
#include <optional>
#include <ranges>
#include <span>

namespace logicsim {

class InvalidLineTreeException : public std::exception {
   public:
    InvalidLineTreeException(const char *message) noexcept;
    auto what() const noexcept -> const char *;

   private:
    const char *message_;
};

[[noreturn]] auto throw_invalid_line_tree_exception(const char *message) -> void;

//
// Done:
//  * create from lines & merging
//  * iter over segments
//  * iter with lengths for each segment
//  * validate inputs
//  * calculate number of outputs
//  * get lengths of each output
//  * position of cross_point dots
//  * get position of each output
//

//
//           / --- c
//  a ---- b
//           \ --- d
//

//
// Design:
// * structure is immutable
// * inputs and outputs are leaf nodes (have only one outgoing edge)
// * composable through merging
//

class LineTree;

using line_tree_vector_t = std::vector<std::reference_wrapper<const LineTree>>;

auto merge(line_tree_vector_t line_trees, std::optional<point_t> new_root = {})
    -> std::optional<LineTree>;

auto merge_lines(std::span<const ordered_line_t> segments) -> std::vector<ordered_line_t>;

template <typename index_t>
class AdjacencyGraph;

class LineTree {
   public:
    class SegmentIterator;
    class SegmentView;

    class InternalPointIterator;
    class InternalPointView;

    struct sized_line_t;
    class SegmentSizeIterator;
    class SegmentSizeView;

    using length_t = int32_t;
    // TODO use segment_index_t
    using index_t = uint16_t;

    using Graph = AdjacencyGraph<LineTree::index_t>;

    explicit constexpr LineTree() = default;
    explicit LineTree(std::initializer_list<point_t> points);

    template <std::ranges::forward_range R>
    explicit LineTree(R &&r);

    template <std::input_iterator I, std::sentinel_for<I> S>
    explicit LineTree(I begin, S end);

    [[nodiscard]] static auto from_segments(std::span<const ordered_line_t> segments,
                                            std::optional<point_t> new_root = {})
        -> std::optional<LineTree>;

    [[nodiscard]] static auto from_points(std::initializer_list<point_t> points)
        -> std::optional<LineTree>;

    template <std::ranges::forward_range R>
    [[nodiscard]] static auto from_points(R &&r) -> std::optional<LineTree>;

    template <std::input_iterator I, std::sentinel_for<I> S>
    [[nodiscard]] static auto from_points(I begin, S end) -> std::optional<LineTree>;

    [[nodiscard]] static auto from_graph(point_t root, const Graph &graph)
        -> std::optional<LineTree>;

    auto swap(LineTree &other) noexcept -> void;

    // return tree with new root, if possible
    [[nodiscard]] auto reroot(const point_t new_root) const -> std::optional<LineTree>;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto segment_count() const noexcept -> std::size_t;
    [[nodiscard]] auto segment(int index) const -> line_t;
    [[nodiscard]] auto segments() const noexcept -> SegmentView;
    [[nodiscard]] auto sized_segments() const noexcept -> SegmentSizeView;
    [[nodiscard]] auto points() const -> std::span<const point_t>;
    // skips input and output points
    [[nodiscard]] auto internal_points() const -> InternalPointView;

    [[nodiscard]] auto input_position() const -> point_t;
    [[nodiscard]] auto input_orientation() const -> orientation_t;
    [[nodiscard]] auto output_count() const -> std::size_t;

    [[nodiscard]] auto output_positions() const {
        return transform_view(output_indices_,
                              [&](index_t index) { return points_.at(index); });
    }

    [[nodiscard]] auto output_position(std::size_t index) const -> point_t;
    [[nodiscard]] auto output_orientation(std::size_t index) const -> orientation_t;
    [[nodiscard]] auto calculate_output_lengths() const -> std::vector<length_t>;

    [[nodiscard]] auto format() const -> std::string;

    auto validate() const -> void;

   private:
    struct backtrack_memory_t;
    class TreeBuilderVisitor;

    template <std::input_iterator I, std::sentinel_for<I> S>
    auto construct_impl(I begin, S end) -> void;
    auto initialize_data_structure() -> void;

    auto validate_points_or_throw() const -> void;
    [[nodiscard]] auto validate_points_error() const
        -> std::optional<InvalidLineTreeException>;
    [[nodiscard]] auto validate_horizontal_follows_vertical() const -> bool;
    [[nodiscard]] auto validate_segments_horizontal_or_vertical() const -> bool;
    [[nodiscard]] auto validate_no_internal_collisions() const -> bool;

    [[nodiscard]] auto segment_points(int index) const -> std::pair<point_t, point_t>;
    [[nodiscard]] auto starts_new_subtree(int index) const -> bool;

    using policy = folly::small_vector_policy::policy_size_type<index_t>;
    using point_vector_t = folly::small_vector<point_t, 2, policy>;
    using index_vector_t = folly::small_vector<index_t, 4, policy>;
    using length_vector_t = folly::small_vector<length_t, 2, policy>;

    static_assert(sizeof(point_vector_t) == 10);
    static_assert(sizeof(index_vector_t) == 10);
    static_assert(sizeof(length_vector_t) == 10);

    point_vector_t points_ {};
    index_vector_t indices_ {};
    length_vector_t lengths_ {};
    index_vector_t output_indices_ {};

   public:
    // TODO make private
    explicit LineTree(point_vector_t points, index_vector_t indices,
                      length_vector_t lengths)
        : points_ {points}, indices_ {indices}, lengths_ {lengths} {};
};

static_assert(sizeof(LineTree) == 40);

auto swap(LineTree &a, LineTree &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::LineTree &a, logicsim::LineTree &b) noexcept -> void;

namespace logicsim {

class LineTree::SegmentIterator {
   public:
    using iterator_concept = std::bidirectional_iterator_tag;
    using iterator_category = std::bidirectional_iterator_tag;

    using value_type = line_t;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    // needs to be default constructable, so ElementView can become a range and view
    SegmentIterator() = default;
    [[nodiscard]] explicit SegmentIterator(const LineTree &line_tree,
                                           index_t index) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    auto operator++() noexcept -> SegmentIterator &;
    auto operator++(int) noexcept -> SegmentIterator;

    auto operator--() noexcept -> SegmentIterator &;
    auto operator--(int) noexcept -> SegmentIterator;

    [[nodiscard]] auto operator==(const SegmentIterator &right) const noexcept -> bool;
    [[nodiscard]] auto operator-(const SegmentIterator &right) const noexcept
        -> difference_type;

    [[nodiscard]] auto is_connected(const SegmentIterator &other) const noexcept -> bool;

   private:
    const LineTree *line_tree_ {};  // can be null, because default constructable
    index_t index_ {};
};

class LineTree::SegmentView {
   public:
    using iterator_type = SegmentIterator;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit SegmentView(const LineTree &line_tree) noexcept;

    [[nodiscard]] auto begin() const noexcept -> iterator_type;
    [[nodiscard]] auto end() const noexcept -> iterator_type;
    [[nodiscard]] auto size() const noexcept -> size_t;

   private:
    gsl::not_null<const LineTree *> line_tree_;
};

//
// Internal Point Iterator & View
//

class LineTree::InternalPointIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using value_type = point_t;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    // needs to be default constructable, so ElementView can become a range and view
    InternalPointIterator() = default;
    [[nodiscard]] explicit InternalPointIterator(const LineTree &line_tree,
                                                 index_t index) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    auto operator++() noexcept -> InternalPointIterator &;
    auto operator++(int) noexcept -> InternalPointIterator;

    [[nodiscard]] auto operator==(const InternalPointIterator &right) const noexcept
        -> bool;

   private:
    const LineTree *line_tree_ {};  // can be null, because default constructable
    index_t index_ {};
};

class LineTree::InternalPointView {
   public:
    using iterator_type = InternalPointIterator;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit InternalPointView(const LineTree &line_tree) noexcept;

    [[nodiscard]] auto begin() const noexcept -> iterator_type;
    [[nodiscard]] auto end() const noexcept -> iterator_type;

   private:
    gsl::not_null<const LineTree *> line_tree_;
};

//
// Size Iterator & View
//

struct LineTree::sized_line_t {
    line_t line;
    length_t p0_length;
    length_t p1_length;
    bool has_cross_point_p0;

    auto operator==(const sized_line_t &other) const noexcept -> bool = default;

    auto format() const -> std::string;
};

class LineTree::SegmentSizeIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using value_type = sized_line_t;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    // needs to be default constructable, so ElementView can become a range and view
    SegmentSizeIterator() = default;
    [[nodiscard]] explicit SegmentSizeIterator(const LineTree &line_tree,
                                               index_t point_index) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    auto operator++() noexcept -> SegmentSizeIterator &;
    auto operator++(int) noexcept -> SegmentSizeIterator;

    [[nodiscard]] auto operator==(const SegmentSizeIterator &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(const SegmentSizeIterator &right) const noexcept
        -> difference_type;

   private:
    const LineTree *line_tree_ {};  // can be null, because default constructable
    length_t start_length_ {};
    index_t point_index_ {};
    index_t length_index_ {};
};

class LineTree::SegmentSizeView {
   public:
    using iterator_type = SegmentSizeIterator;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit SegmentSizeView(const LineTree &line_tree) noexcept;

    [[nodiscard]] auto begin() const noexcept -> iterator_type;
    [[nodiscard]] auto end() const noexcept -> iterator_type;
    [[nodiscard]] auto size() const noexcept -> size_t;

   private:
    gsl::not_null<const LineTree *> line_tree_;
};

}  // namespace logicsim

static_assert(std::bidirectional_iterator<logicsim::LineTree::SegmentIterator>);
static_assert(std::forward_iterator<logicsim::LineTree::InternalPointIterator>);
static_assert(std::forward_iterator<logicsim::LineTree::SegmentSizeIterator>);

template <>
inline constexpr bool std::ranges::enable_view<logicsim::LineTree::SegmentView> = true;

template <>
inline constexpr bool std::ranges::enable_view<logicsim::LineTree::InternalPointIterator>
    = true;

template <>
inline constexpr bool std::ranges::enable_view<logicsim::LineTree::SegmentSizeView>
    = true;

//
// Implementation
//

namespace logicsim {

template <std::ranges::forward_range R>
LineTree::LineTree(R &&r) : LineTree {std::ranges::begin(r), std::ranges::end(r)} {}

template <std::input_iterator I, std::sentinel_for<I> S>
LineTree::LineTree(I begin, S end) {
    construct_impl(begin, end);
    validate_points_or_throw();
}

template <std::input_iterator I, std::sentinel_for<I> S>
auto LineTree::construct_impl(I begin, S end) -> void {
    points_ = point_vector_t {begin, end};
    initialize_data_structure();
}

template <std::ranges::forward_range R>
auto LineTree::from_points(R &&r) -> std::optional<LineTree> {
    return LineTree::from_points(std::ranges::begin(r), std::ranges::end(r));
}

template <std::input_iterator I, std::sentinel_for<I> S>
auto LineTree::from_points(I begin, S end) -> std::optional<LineTree> {
    auto line_tree = std::optional<LineTree> {LineTree {}};
    line_tree->construct_impl(begin, end);

    if (line_tree->validate_points_error().has_value()) {
        return std::nullopt;
    }

    return line_tree;
}

}  // namespace logicsim

#endif