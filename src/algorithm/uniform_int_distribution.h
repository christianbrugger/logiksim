#ifndef LOGICSIM_RANDOM_UNIFORM_INT_DISTRIBUTION_H
#define LOGICSIM_RANDOM_UNIFORM_INT_DISTRIBUTION_H

#include <boost/random/uniform_int_distribution.hpp>

namespace logicsim {

template <typename T>
[[nodiscard]] auto uint_distribution(T min, T max)
    -> boost::random::uniform_int_distribution<T> {
    return boost::random::uniform_int_distribution<T> {min, max};
}

}  // namespace logicsim

#endif
