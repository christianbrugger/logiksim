#ifndef LOGICSIM_RANDOM_SEGMENT_H
#define LOGICSIM_RANDOM_SEGMENT_H

#include "random/generator.h"

namespace logicsim {

struct element_id_t;
struct segment_index_t;
struct segment_t;
struct segment_part_t;

class Layout;
class SegmentTree;

/**
 * @brief: finds tree with at least one segment or returns null_element
 */
[[nodiscard]] auto get_random_segment_tree(Rng& rng, const Layout& layout)
    -> element_id_t;
[[nodiscard]] auto get_random_segment(Rng& rng, const SegmentTree& tree)
    -> segment_index_t;
[[nodiscard]] auto get_random_segment(Rng& rng, const Layout& layout) -> segment_t;
[[nodiscard]] auto get_random_segment_part(Rng& rng, const Layout& layout)
    -> segment_part_t;

auto add_random_segment(Rng& rng, SegmentTree& tree) -> segment_index_t;

}  // namespace logicsim

#endif
