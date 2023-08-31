#ifndef LOGIKSIM_RANDOM_H
#define LOGIKSIM_RANDOM_H

#include "vocabulary.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace logicsim {

using Rng = boost::random::mt19937;

auto get_random_number_generator() -> Rng;
auto get_random_number_generator(uint32_t seed) -> Rng;

template <typename T>
auto uint_distribution(T min, T max) -> boost::random::uniform_int_distribution<T> {
    return boost::random::uniform_int_distribution<T> {min, max};
}

auto get_random_bool(Rng& rng) -> bool;
auto get_random_bool(Rng& rng, double percentage) -> bool;

auto get_random_grid(Rng& rng, grid_t::value_type min = grid_t::min(),
                     grid_t::value_type max = grid_t::max()) -> grid_t;
auto get_random_point(Rng& rng, grid_t::value_type min = grid_t::min(),
                      grid_t::value_type max = grid_t::max()) -> point_t;
auto get_random_point(Rng& rng, rect_t rect) -> point_t;

auto get_random_line(Rng& rng, grid_t::value_type min = grid_t::min(),
                     grid_t::value_type max = grid_t::max()) -> ordered_line_t;
auto get_random_line(Rng& rng, grid_t::value_type min, grid_t::value_type max,
                     grid_t::value_type max_length) -> ordered_line_t;

auto get_random_lines(Rng& rng, std::size_t count, grid_t::value_type min = grid_t::min(),
                      grid_t::value_type max = grid_t::max())
    -> std::vector<ordered_line_t>;

auto get_random_part(Rng& rng, part_t full_part) -> part_t;
auto get_random_part(Rng& rng, ordered_line_t line) -> part_t;

auto get_random_insertion_mode(Rng& rng) -> InsertionMode;

// Segment Trees
class Layout;
class SegmentTree;

// finds tree with at least one segment or returns null_element
auto get_random_segment_tree(Rng& rng, const Layout& layout) -> element_id_t;
auto get_random_segment(Rng& rng, const SegmentTree& tree) -> segment_index_t;
auto get_random_segment(Rng& rng, const Layout& layout) -> segment_t;
auto get_random_segment_part(Rng& rng, const Layout& layout) -> segment_part_t;

}  // namespace logicsim

#endif