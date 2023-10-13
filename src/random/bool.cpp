#include "random/bool.h"

#include "algorithm/round.h"
#include "algorithm/uniform_int_distribution.h"

#include <boost/random/discrete_distribution.hpp>
#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

auto get_random_bool(Rng& rng) -> bool {
    return uint_distribution(0, 1)(rng) > 0;
}

auto get_random_bool(Rng& rng, double percentage) -> bool {
    if (percentage < 0.0 || percentage > 1.0) {
        throw std::runtime_error("percentage needs to be between 0 and 1");
    }
    // auto dist = boost::random::discrete_distribution<>({percentage, 1.0 - percentage});
    // return dist(rng) == 0;

    // mantissa size
    constexpr static auto max_value = uint64_t {9007199254740992};  // 2 ** 53

    const auto max_double = gsl::narrow<double>(max_value);
    const auto threshold = gsl::narrow<uint64_t>(round_fast(max_double * percentage));

    return uint_distribution(uint64_t {0}, max_value)(rng) <= threshold;
}

}  // namespace logicsim
