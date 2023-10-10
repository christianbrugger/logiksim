#ifndef LOGICSIM_RANDOM_GENERATOR_H
#define LOGICSIM_RANDOM_GENERATOR_H

#include <boost/random/mersenne_twister.hpp>

#include <cstdint>

namespace logicsim {

using Rng = boost::random::mt19937;

auto get_random_number_generator() -> Rng;
auto get_random_number_generator(uint32_t seed) -> Rng;

}  // namespace logicsim

#endif
