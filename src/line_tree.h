#ifndef LOGIKSIM_LINETREE_H
#define LOGIKSIM_LINETREE_H

#include "geometry.h"

#include <folly/small_vector.h>

namespace logicsim {

//
//           / --- c
//  a ---- b
//           \ --- d
//

class LineTree {
   public:
    using index_t = uint16_t;

    using point_vector_t = folly::small_vector<point2d_t, 2, uint16_t>;
    using index_vector_t = folly::small_vector<index_t, 4, uint16_t>;

    static_assert(sizeof(point_vector_t) == 10);
    static_assert(sizeof(index_vector_t) == 10);

   public:
    explicit LineTree() = default;
    explicit LineTree(point_vector_t points, index_vector_t indices);

   private:
    point_vector_t points_ {};
    index_vector_t indices_ {};
};

}  // namespace logicsim

#endif