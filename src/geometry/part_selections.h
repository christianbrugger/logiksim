#ifndef LOGICSIM_GEOMETRY_PART_SELECTIONS_H
#define LOGICSIM_GEOMETRY_PART_SELECTIONS_H

namespace logicsim {

struct part_t;
class PartSelection;

[[nodiscard]] auto a_overlaps_any_of_b(part_t a, const PartSelection& b) -> bool;

[[nodiscard]] auto a_overlaps_any_of_b(const PartSelection& a, const PartSelection& b)
    -> bool;
[[nodiscard]] auto a_disjoint_of_b(const PartSelection& a, const PartSelection& b)
    -> bool;

}  // namespace logicsim

#endif
