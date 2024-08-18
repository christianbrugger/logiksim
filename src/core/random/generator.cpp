#include "random/generator.h"

#include <boost/random/seed_seq.hpp>

#include <array>
#include <random>

namespace logicsim {

auto get_random_number_generator() -> Rng {
    auto rd = std::random_device {};
    const auto seed_values = std::array {rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};

    const auto seed_seq = boost::random::seed_seq(seed_values.begin(), seed_values.end());
    return Rng {seed_seq};
}

auto get_random_number_generator(uint32_t seed) -> Rng {
    return Rng {seed};
}

}  // namespace logicsim
