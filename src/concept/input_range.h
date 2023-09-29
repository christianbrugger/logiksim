#ifndef LOGICSIM_CONCEPT_INPUT_RANGE_H
#define LOGICSIM_CONCEPT_INPUT_RANGE_H

#include <ranges>
#include <type_traits>

namespace logicsim {

template <typename R, typename V>
concept input_range_of = std::ranges::input_range<R> &&
                         std::convertible_to<std::ranges::range_reference_t<R>, V>;

}

#endif
