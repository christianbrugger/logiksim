#ifndef LOGIKSIM_LINETREE_H
#define LOGIKSIM_LINETREE_H

#include "geometry.h"

#include <fmt/core.h>
#include <folly/small_vector.h>
#include <gsl/gsl>

#include <functional>
#include <optional>
#include <ranges>

namespace logicsim {

//
// TODO:
//  * position of connector dots
//  * get position of each output
//
// Done:
//  * create from lines & merging
//  * iter over segments
//  * iter with lengths for each segment
//  * validate inputs
//  * calculate number of outputs
//  * get lengths of each output
//

//
//           / --- c
//  a ---- b
//           \ --- d
//

// first point is input
// structure is immutable, create new tree, if it needs to be changed
// outputs need to be leaf nodes (cannot have outgoing connections)
// class does not know about simulation or circuit (reduce coupling)

class LineTree;

using line_tree_vector_t = std::vector<std::reference_wrapper<LineTree>>;

auto merge(line_tree_vector_t line_trees, std::optional<point2d_t> new_root = {})
    -> std::optional<LineTree>;

template <typename index_t>
class AdjacencyGraph;

class LineTree {
   public:
    class SegmentIterator;
    class SegmentView;

    struct sized_line2d_t;
    class SegmentSizeIterator;
    class SegmentSizeView;

    using length_t = int32_t;
    using index_t = uint16_t;

    using Graph = AdjacencyGraph<LineTree::index_t>;

    explicit LineTree() = default;
    explicit LineTree(std::initializer_list<point2d_t> points);

    [[nodiscard]] static auto from_graph(point2d_t root, const Graph &graph)
        -> std::optional<LineTree>;

    // return tree with new root, if possible
    [[nodiscard]] auto reroot(const point2d_t new_root) const -> std::optional<LineTree>;

    [[nodiscard]] auto input_point() const -> point2d_t;

    [[nodiscard]] auto segment_count() const noexcept -> int;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto segment(int index) const -> line2d_t;
    [[nodiscard]] auto segments() const noexcept -> SegmentView;
    [[nodiscard]] auto sized_segments() const noexcept -> SegmentSizeView;

    [[nodiscard]] auto output_count() const -> int;
    [[nodiscard]] auto output_delays() const -> std::vector<length_t>;

   private:
    struct backtrack_memory_t;
    class TreeBuilderVisitor;

    [[nodiscard]] auto validate_horizontal_follows_vertical() const -> bool;
    [[nodiscard]] auto validate_segments_horizontal_or_vertical() const -> bool;
    [[nodiscard]] auto validate_no_internal_collisions() const -> bool;

    [[nodiscard]] auto starts_new_subtree(int index) const -> bool;

    // TODO consider merging for better locality
    using policy = folly::small_vector_policy::policy_size_type<index_t>;
    using point_vector_t = folly::small_vector<point2d_t, 2, policy>;
    using index_vector_t = folly::small_vector<index_t, 4, policy>;
    using length_vector_t = folly::small_vector<length_t, 2, policy>;

    static_assert(sizeof(point_vector_t) == 10);
    static_assert(sizeof(index_vector_t) == 10);
    static_assert(sizeof(length_vector_t) == 10);

    point_vector_t points_ {};
    index_vector_t indices_ {};
    length_vector_t lengths_ {};

   public:
    explicit LineTree(point_vector_t points, index_vector_t indices,
                      length_vector_t lengths)
        : points_ {points}, indices_ {indices}, lengths_ {lengths} {};
};

[[noreturn]] auto throw_invalid_line_tree_exception(const char *message) -> void;

class InvalidLineTreeException : public std::exception {
   public:
    InvalidLineTreeException(const char *message) noexcept;
    auto what() const noexcept -> const char *;

   private:
    const char *message_;
};

class LineTree::SegmentIterator {
   public:
    using iterator_concept = std::bidirectional_iterator_tag;
    using iterator_category = std::bidirectional_iterator_tag;

    using value_type = line2d_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type;
    // TODO check if reference needs to be return type of operator*
    using reference = value_type;
    // using reference = value_type &;
    // TODO also check pointer, if we need it, needs to be return of -> operator
    //    https://vector-of-bool.github.io/2020/06/13/cpp20-iter-facade.html

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
// Size Iterator & View
//

struct LineTree::sized_line2d_t {
    line2d_t line;
    length_t p0_length;
    length_t p1_length;

    auto operator==(sized_line2d_t other) const noexcept {
        return line == other.line && p0_length == other.p0_length
               && p1_length == other.p1_length;
    }
};

class LineTree::SegmentSizeIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using value_type = sized_line2d_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type;
    // TODO check if reference needs to be return type of operator*
    using reference = value_type;
    // using reference = value_type &;
    // TODO also check pointer, if we need it, needs to be return of -> operator
    //    https://vector-of-bool.github.io/2020/06/13/cpp20-iter-facade.html

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

static_assert(std::forward_iterator<logicsim::LineTree::SegmentIterator>);
static_assert(std::forward_iterator<logicsim::LineTree::SegmentSizeIterator>);

template <>
inline constexpr bool std::ranges::enable_view<logicsim::LineTree::SegmentView> = true;

template <>
inline constexpr bool std::ranges::enable_view<logicsim::LineTree::SegmentSizeView>
    = true;

template <>
struct fmt::formatter<logicsim::LineTree::sized_line2d_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::LineTree::sized_line2d_t &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "SizedLine({}, {}, {}, {})", obj.line.p0,
                              obj.line.p1, obj.p0_length, obj.p1_length);
    }
};

#endif