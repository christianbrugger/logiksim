#ifndef LOGICSIM_CONCEPT_RANGE_VALUE_TYPE_H
#define LOGICSIM_CONCEPT_RANGE_VALUE_TYPE_H

#include <concepts>
#include <iterator>

namespace logicsim {

template <typename T>
concept range_value_type = std::weakly_incrementable<T> && std::equality_comparable<T> &&
                           std::totally_ordered<T>;

}

#endif
