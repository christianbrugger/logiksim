#ifndef LOGICSIM_INDEX_SPATIAL_POINT_INDEX_H
#define LOGICSIM_INDEX_SPATIAL_POINT_INDEX_H

#include "core/container/value_pointer.h"
#include "core/format/container.h"
#include "core/format/struct.h"
#include "core/vocabulary/point.h"

#include <folly/small_vector.h>

namespace logicsim {

struct ordered_line_t;

namespace spatial_point_index {
struct tree_container;

using point_vector_t = folly::small_vector<point_t, 18>;
static_assert(sizeof(point_vector_t) == 80);
}  // namespace spatial_point_index

extern template class value_pointer<spatial_point_index::tree_container>;

class SpatialPointIndex {
   public:
    using point_vector_t = spatial_point_index::point_vector_t;

   public:
    [[nodiscard]] explicit SpatialPointIndex() = default;
    [[nodiscard]] explicit SpatialPointIndex(std::span<const point_t> points);

    [[nodiscard]] auto format() const -> std::string;

    auto add_split_point(point_t point) -> void;

    [[nodiscard]] auto query_is_inside(ordered_line_t line) const -> point_vector_t;
    [[nodiscard]] auto query_intersects(ordered_line_t line) const -> point_vector_t;

   private:
    value_pointer<spatial_point_index::tree_container> tree_;
};

static_assert(std::semiregular<SpatialPointIndex>);

}  // namespace logicsim

#endif
