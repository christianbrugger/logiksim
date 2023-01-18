

#include "line_tree.h"

namespace logicsim {

LineTree::LineTree(point_vector_t points, index_vector_t indices)
    : points_ {std::move(points)}, indices_ {std::move(indices)} {}

}  // namespace logicsim
