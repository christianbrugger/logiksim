#include "core/random/part.h"

#include "core/algorithm/uniform_int_distribution.h"
#include "core/geometry/part.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/part.h"

namespace logicsim {

auto get_random_part(Rng& rng, part_t full_part) -> part_t {
    auto begin = offset_t::value_type {};
    auto end = offset_t::value_type {};
    const auto part_dist = uint_distribution(full_part.begin.value, full_part.end.value);
    while (begin >= end) {
        begin = part_dist(rng);
        end = part_dist(rng);
    }
    return part_t {offset_t {begin}, offset_t {end}};
}

auto get_random_part(Rng& rng, ordered_line_t line) -> part_t {
    const auto full_part = to_part(line);
    return get_random_part(rng, full_part);
}

}  // namespace logicsim
