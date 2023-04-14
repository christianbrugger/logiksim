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

auto get_random_grid(Rng& rng, grid_t::value_type min = grid_t::min(),
                     grid_t::value_type max = grid_t::max()) -> grid_t;

auto get_random_line(Rng& rng, grid_t::value_type min = grid_t::min(),
                     grid_t::value_type max = grid_t::max()) -> ordered_line_t;

auto get_random_part(Rng& rng, part_t full_part) -> part_t;
auto get_random_part(Rng& rng, ordered_line_t line) -> part_t;

auto get_random_insertion_mode(Rng& rng) -> InsertionMode;

}  // namespace logicsim

#endif