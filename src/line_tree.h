#ifndef LOGIKSIM_LINETREE_H
#define LOGIKSIM_LINETREE_H

#include "geometry.h"

#include <folly/small_vector.h>
#include <gsl/gsl>

#include <optional>

namespace logicsim {

// TODO interface:
//  * create from what?
//  * iter over segments
//  * iter with lengths for each segment
//  * get lengths of each output
//  * position of connector dots
//  * calculate number of outputs
//  * validation
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

class LineTree {
   public:
    class SegmentIterator;
    class SegmentView;

    explicit LineTree() = default;
    explicit LineTree(std::initializer_list<point2d_t> points);

    auto validate() const -> bool;
    auto segment_count() const noexcept -> int;
    auto segment(int index) const -> line2d_t;
    auto segments() const noexcept -> SegmentView;

   private:
    auto validate_segments_horizontal_or_vertical() const -> bool;
    auto validate_no_internal_collisions() const -> bool;
    auto validate_no_unecessary_points() const -> bool;

    using index_t = uint16_t;
    using length_t = uint32_t;

    using point_vector_t = folly::small_vector<point2d_t, 2, uint16_t>;
    using index_vector_t = folly::small_vector<index_t, 4, uint16_t>;
    using length_vector_t = folly::small_vector<length_t, 2, uint16_t>;

    static_assert(sizeof(point_vector_t) == 10);
    static_assert(sizeof(index_vector_t) == 10);
    static_assert(sizeof(length_vector_t) == 10);

    point_vector_t points_ {};
    index_vector_t indices_ {};
    length_vector_t lengths_ {};
};

class LineTree::SegmentIterator {
   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = line2d_t;
    using difference_type = size_t;
    using pointer = value_type *;
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

    [[nodiscard]] auto operator==(const SegmentIterator &right) const noexcept -> bool;
    [[nodiscard]] auto operator-(const SegmentIterator &right) const noexcept
        -> difference_type;

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

   private:
    gsl::not_null<const LineTree *> line_tree_;
};

}  // namespace logicsim

#endif